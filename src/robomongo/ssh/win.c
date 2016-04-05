

int ssh_win_init() {
    WSADATA wsadata;

    err = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (err != 0) {
        log_error("WSAStartup failed with error: %d", err);
        return 1;
    }
}
