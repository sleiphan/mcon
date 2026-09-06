#include <stdint.h>
#include <argp.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>



static struct argp_option cli_options[] = {
    {"port",         'p',  "PORT", 0, "The port that the client should try connecting to."},
    {"host",         'H',  "HOST", 0, "The address that the client should try connecting to."},
    {"source-range", 's', "RANGE", 0, "The IPv4 source address range to use when the amount of connections exceed 50k. Specified in CIDR notation."},
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

int parse_port(const char* arg) {
    char* end;
    errno = 0;

    long port = strtol(arg, &end, 10);

    if (errno == ERANGE || end == arg || *end != '\0')
        return -1;
    
    if (port < 0 || port > UINT16_MAX)
        return -1;

    return (int) port;
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

    if (errno == ERANGE ||
        end == arg + delimiter_index + 1 ||
        *end != '\0' ||
        prefix < 0 ||
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

    range_out->first = (struct in_addr) {.s_addr = htonl(address_host & ~host_mask)};
    range_out->final = (struct in_addr) {.s_addr = htonl(address_host | host_mask)};

    return 0;
}

static error_t parse_args(int key, char* arg, struct argp_state *state) {
    struct cli_config* config = state->input;

    switch (key) {
        case 'p':
            int port = parse_port(arg);
            if (port == -1)
                return ARGP_ERR_UNKNOWN;
            config->port = (uint16_t) port;
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



// struct command {
//     const char *name;
//     int (*handler)(int argc, char **argv);
// };

// static const struct command commands[] = {
//     { "add",   cmd_add },
//     { "close", cmd_close },
//     { "list",  cmd_list },
//     { "quit",  cmd_quit },
// };

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
", host_str, args.port, first_str, final_str);
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
}
