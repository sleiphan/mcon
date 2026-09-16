#include <argp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static struct argp_option cli_options[] = {
    {"port", 'p', "PORT", 0, "The port that the client should try connecting to."},
    {"host", 'H', "HOST", 0, "The address that the client should try connecting to."},
    {"source-range", 's', "RANGE", 0,
     "The IPv4 source address range to use when the amount of connections exceed 50k. Specified in "
     "CIDR notation."},
};

struct ipv4_range {
    struct in_addr first;
    struct in_addr final;
};

struct cli_config {
    uint16_t port;
    struct in_addr host;
    struct ipv4_range source_range;
};

struct cli_config cli_config_default() {
    struct cli_config args;

    args.host.s_addr = INADDR_ANY;
    args.port = 8080;
    args.source_range.first.s_addr = inet_addr("127.0.0.2");
    args.source_range.final.s_addr = inet_addr("127.0.0.22");

    return args;
}

int parse_port(const char *arg) {
    char *end;
    errno = 0;

    long port = strtol(arg, &end, 10);

    if (errno == ERANGE || end == arg || *end != '\0')
        return -1;

    if (port < 0 || port > UINT16_MAX)
        return -1;

    return (int)port;
}

int parse_source_range(const char *arg, struct ipv4_range *range_out) {
    const size_t arg_len = strlen(arg);

    size_t delimiter_index = 0;
    while (delimiter_index < arg_len && arg[delimiter_index] != '/')
        delimiter_index++;

    if (delimiter_index == arg_len)
        return -1;

    char *end;
    errno = 0;

    long prefix = strtol(arg + delimiter_index + 1, &end, 10);

    if (errno == ERANGE || end == arg + delimiter_index + 1 || *end != '\0' || prefix < 0 ||
        prefix > 32)
        return -1;

    struct in_addr address;
    char address_str[INET_ADDRSTRLEN + 1];
    memcpy(address_str, arg, delimiter_index);
    address_str[delimiter_index] = '\0';
    if (inet_pton(AF_INET, address_str, &address) != 1)
        return -1;

    uint32_t address_host = ntohl(address.s_addr);

    uint32_t host_mask;
    if (prefix == 0)
        host_mask = UINT32_MAX;
    else
        host_mask = UINT32_MAX >> prefix;

    range_out->first = (struct in_addr){.s_addr = htonl(address_host)};
    range_out->final = (struct in_addr){.s_addr = htonl(address_host | host_mask)};

    return 0;
}

static error_t parse_args(int key, char *arg, struct argp_state *state) {
    struct cli_config *config = state->input;

    switch (key) {
    case 'p':
        int port = parse_port(arg);
        if (port == -1)
            return ARGP_ERR_UNKNOWN;
        config->port = (uint16_t)port;
        break;

    case 'H':
        if (inet_pton(AF_INET, arg, &config->host) != 1)
            return ARGP_ERR_UNKNOWN;
        break;

    case 's':
        if (parse_source_range(arg, &config->source_range) != 0)
            return ARGP_ERR_UNKNOWN;
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {
    .options = cli_options,
    .parser = parse_args,
};

struct connection {
    int client_fd;
};

struct client_state {
    struct connection *connections;
    unsigned int connection_count;
};

int connect_to_server(const struct in_addr address, const uint16_t port,
                      const struct in_addr *source_address) {
    struct sockaddr_in server_address_sock;
    memset(&server_address_sock, 0, sizeof(server_address_sock));
    server_address_sock.sin_family = AF_INET;
    server_address_sock.sin_addr = address;
    server_address_sock.sin_port = htons(port);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        return -1;

    if (source_address) {
        struct sockaddr_in source_address_sock = {0};
        source_address_sock.sin_family = AF_INET;
        source_address_sock.sin_addr = *source_address;

        if (bind(client_fd, (const struct sockaddr *)&source_address_sock,
                 sizeof(source_address_sock))) {
            close(client_fd);
            return -1;
        }
    }

    const int connect_rc =
        connect(client_fd, (struct sockaddr *)&server_address_sock, sizeof(server_address_sock));
    if (connect_rc != 0)
        return -1;

    return client_fd;
}

int ping_server(struct cli_config args) {
    int client_fd = connect_to_server(args.host, args.port, NULL);
    if (client_fd < 0)
        return -1;

    shutdown(client_fd, SHUT_WR);
    close(client_fd);

    return 0;
}

int cli_ping_server(int argc, char **argv, struct cli_config args, struct client_state *state) {
    if (argc > 1) {
        printf("Too many arguments\n");
        return 0;
    }

    const int rc = ping_server(args);
    if (rc == 0) {
        printf("Ping successful; server responded\n");
    } else {
        perror("ping_server");
    }

    return 0;
}

int cli_exit(int argc, char **argv, struct cli_config args, struct client_state *state) {
    if (argc > 1) {
        printf("Too many arguments\n");
        return 0;
    }

    return 1;
}

int get_source_address(const unsigned int connection_idx, const struct ipv4_range source_range,
                       struct in_addr *out) {
    static const unsigned int CONNECTIONS_PER_SOURCE_ADDRESS = 25000;

    const uint32_t min_address = ntohl(source_range.first.s_addr);
    const uint32_t max_address = ntohl(source_range.final.s_addr);
    const uint32_t num_addresses = max_address - min_address;

    // Fail if we don't have enough source addresses
    if (connection_idx >= (num_addresses * CONNECTIONS_PER_SOURCE_ADDRESS)) {
        errno = EINVAL;
        return -1;
    }

    const unsigned int address_idx = connection_idx / CONNECTIONS_PER_SOURCE_ADDRESS;
    const uint32_t target_address = min_address + address_idx;

    out->s_addr = htonl(target_address);
    return 0;
}

bool iteration_timing(const time_t interval_ms, struct timespec *last_exectution) {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    const time_t time_passed_ms = (current_time.tv_sec - (*last_exectution).tv_sec) * 1000 +
                                  (current_time.tv_nsec - (*last_exectution).tv_nsec) / 1000000;

    bool execute = time_passed_ms >= interval_ms;
    *last_exectution = execute ? current_time : *last_exectution;

    return execute;
}

int cli_connect(int argc, char **argv, struct cli_config args, struct client_state *state) {
    if (argc > 2) {
        printf("Too many arguments\n");
        return 0;
    }

    if (argc < 2) {
        printf("Too few arguments\n");
        return 0;
    }

    char *end;
    const long target_connections = strtol(argv[1], &end, 10);

    if (target_connections < 0) {
        printf("Invalid argument\n");
        return 0;
    }

    if (target_connections > 1000000) {
        printf("Too many connections\n");
        return 0;
    }

    if (target_connections == state->connection_count) {
        printf("%d connections already active\n", target_connections);
        return 0;
    }

    if (state->connection_count < target_connections) {
        struct in_addr source_address;
        if (get_source_address(target_connections - 1, args.source_range, &source_address)) {
            printf("Not enough source addresses\n");
            return 0;
        }

        struct connection *new_mem =
            realloc(state->connections, target_connections * sizeof(struct connection));
        if (new_mem == NULL) {
            perror("realloc");
            return 0;
        }
        state->connections = new_mem;

        struct timespec last_excecution;
        clock_gettime(CLOCK_MONOTONIC, &last_excecution);
        printf("%d / %d", state->connection_count, target_connections);

        int connections_established = 0;
        for (int i = state->connection_count; i < target_connections; i++) {
            get_source_address(i, args.source_range, &source_address);

            int client_fd = connect_to_server(args.host, args.port, &source_address);
            if (client_fd < 0) {
                perror("connect_to_server");
                break;
            }

            connections_established++;
            state->connections[i].client_fd = client_fd;

            if (iteration_timing(50, &last_excecution)) {
                printf("\r\033[K %d / %d", state->connection_count + connections_established,
                       target_connections);
                fflush(stdout);
            }
        }

        printf("\nSuccessfully connected %d new clients\n", connections_established);
        state->connection_count += connections_established;
        printf("Current total: %d\n", state->connection_count);

    } else {
        for (int i = state->connection_count - 1; i >= target_connections; i--) {
            shutdown(state->connections[i].client_fd, SHUT_WR);
            close(state->connections[i].client_fd);
        }

        struct connection *new_mem =
            realloc(state->connections, target_connections * sizeof(struct connection));
        if (new_mem == NULL && target_connections != 0) {
            perror("realloc");
            return 0;
        }

        state->connections = new_mem;
        state->connection_count = target_connections;
        printf("Current total: %d\n", state->connection_count);
    }

    return 0;
}

struct command {
    const char *name;
    int (*handler)(int argc, char **argv, struct cli_config cli_args, struct client_state *state);
};

static const struct command commands[] = {
    {"ping", cli_ping_server}, {"status", NULL},  {"connect", cli_connect},
    {"write", NULL},           {"request", NULL}, {"exit", cli_exit},
};

int command_line_loop(struct cli_config args) {
    char *command_str = NULL;
    size_t command_length = 0;
    const unsigned int command_count = sizeof(commands) / sizeof(struct command);
    char *argv[20];
    unsigned int argc = 0;

    struct client_state state = {
        .connection_count = 0,
        .connections = malloc(1),
    };

    bool running = true;
    while (running) {
        if (getline(&command_str, &command_length, stdin) < 0) {
            perror("getline");
            return -1;
        }

        char *savetoken;
        char *token = strtok_r(command_str, " \t\n", &savetoken);
        argc = 0;
        while (token != NULL && argc < (sizeof(argv) / sizeof(char *))) {
            argv[argc++] = token;
            token = strtok_r(NULL, " \t\n", &savetoken);
        }

        bool unknown_command = false;
        for (int i = 0; i < command_count; i++) {
            if (strncmp(argv[0], commands[i].name, strlen(commands[i].name)) == 0) {
                const int command_rc = commands[i].handler(argc, argv, args, &state);
                if (command_rc == 1)
                    running = false;
                break;
            }

            unknown_command = i == command_count - 1;
        }

        if (unknown_command)
            printf("Unknown command: %s\n", argv[0]);
    }
}

void print_run_config(struct cli_config args) {
    char host_str[INET_ADDRSTRLEN];
    char first_str[INET_ADDRSTRLEN];
    char final_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &args.host, host_str, sizeof(host_str));
    inet_ntop(AF_INET, &args.source_range.first, first_str, sizeof(first_str));
    inet_ntop(AF_INET, &args.source_range.final, final_str, sizeof(final_str));

    printf("\
host         = %s\n\
port         = %d\n\
source_range = %s - %s\n\
",
           host_str, args.port, first_str, final_str);
}

int main(int argc, char **argv) {
    // The program starts and will hold connections while it runs.
    // Commands are submitted to the program, each line living in its
    // own lifecycle. The caller will be able to increase and decrease
    // the amount of connections, and query status values from the server.

    struct cli_config args = cli_config_default();
    argp_parse(&argp, argc, argv, 0, 0, &args);
    print_run_config(args);

    //
    const int rc = ping_server(args);
    if (rc == 0)
        printf("Ping successful; server responded\n");
    else {
        perror("ping_server");
        return -1;
    }

    return command_line_loop(args);
}
