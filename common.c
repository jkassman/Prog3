void errorCheckRecv(int sock, void *data, size_t data_len, const char *errorMsg) 
{
    int status;
    status = recv(sock, (char*) data, data_len, 0);
    if (status < 0) 
    {
        perror(errorMsg);
        close(sock);
        exit(8);
    }
}

void errorCheckSend(int sock, void *data, size_t data_len, const char *errorMsg)
{
    int status;
    status = send(sock, (char*) data, data_len, 0);
    if (status < 0)
    {
        perror(errorMsg);
        close(sock);
        exit(9);
    }
}

void errorCheckStrSend(int sock, char *stringToSend, const char *errorMsg)
{
    int status;
    status = send(sock, stringToSend, strlen(stringToSend) + 1, 0);
    if (status < 0)
    {
        perror(errorMsg);
        close(sock);
        exit(10);
    }
}
