# Make all scripts executable
find . -type f -name '*.sh' -exec chmod +x {} \;

# Run GitHub login script if not already logged in
.devcontainer/github-login.sh
