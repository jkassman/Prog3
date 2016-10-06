#include "common.h"

//The following sends and receives just make the code cleaner.
//They all exit the *entire program* on an error and print out an error message.
//Also, they're void * so you don't have to cast (char*) anymore.

/*
  Receives data_len bytes from sock and stores it in data.
  Prints errorMsg and exits the *entire program* if any error occors.

  sock: The socket to receive data over.
  data: A buffer into which at most data_len data will be placed.
  data_len: size in bytes of the data to receive.
  errorMsg: What to print out (perror) before exiting if something goes wrong.
*/
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

/*
  Sends the buffer, data, of data_len bytes out of sock.
  Prints errorMsg and exits the *entire program* if any error occors.

  sock: The socket to send data over.
  data: A buffer of size data_len that will be sent.
  data_len: size in bytes of the data to send.
  errorMsg: What to print out (perror) before exiting if something goes wrong.
*/
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
/*
  Exactly the same as errorCheckSend except it uses strlen() as data_len.
 */
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

/* stringToInt()
 Convert the string to a long in base 10 using strtol,
 If out of bounds (minVal and maxVal), 
      return an error: 1 if too low, 2 if too high.
 If any unrecognized characters, return 3.
 if successful, return 0.

 destInt: pointer to the int that will be overwritten with the value of the
     integer from the string.
*/
int stringToInt(int *destInt,char *toConvert, int minVal, int maxVal)
{
    char *errorChecker = NULL;
    long long veryLong = strtol(toConvert, &errorChecker, 10);
    if (errorChecker != NULL && *errorChecker != '\0')
    { //unrecognized character
        return 3;
    }
    if (veryLong > maxVal)
    { 
        return 2;
    }
    else if (veryLong < minVal)
    {
        return 1;
    }
    *destInt = (int) veryLong;
    return 0;
}

/*
  Generates an MD5 hash from the file and stores it in hash.

  hash[16]: A buffer of at least 16 bytes that will be overwritten to store
      the MD5 hash of the file.
  fileToHash: Pretty self-explanitory. Note: this function will reset the file
      pointer to the beginning of the file before and after hashing.
 */
void hashFile(unsigned char hash[16], FILE* fileToHash)
{
    rewind(fileToHash);
    //find the MD5 hash of the file:
    MHASH td;
    unsigned char buffer;
    td = mhash_init(MHASH_MD5);
    if (td == MHASH_FAILED) 
    {
        perror("myftpd: mhash_init()");
        exit(4);
    }
  
    while (fread(&buffer, 1, 1, fileToHash) == 1) 
    {
        mhash(td, &buffer, 1);
    }
  
    //finalize hash generation
    mhash_deinit(td, hash);
    rewind(fileToHash);
}

//Returns 1 if hashes are NOT equal
int hashCompare(unsigned char *hash1, unsigned char *hash2) {
    int i;
    for(i = 0; i < 16; i++) {
        if(hash1[i] != hash2[i]) {
            return 1;
        }
    }
    return 0;
}

/*
  Sends a file over the socket, one PROG3_BUFF_SIZE chunk at a time.

  sock: the socket to send the file over
  file: the file you want to send. Note: Does NOT reset file pointer!
  fileSize: the size (in bytes) of the file.
  programName: For error messages only.
       For example, if programName is "myProgram", this may print out 
      "myProgram Error: read file: file does not exist"
      -Required because both client (myftp) and server (myftpd) use this.
 */
void sendFile(int sock, FILE* file, unsigned int fileSize, 
              const char *programName)
{
    char sendFileBuffer[PROG3_BUFF_SIZE];
    size_t bytesRead = 0;
    size_t bytesThisTime = 0;
    int status;

    while (bytesRead < fileSize) 
    {
        //Read up to the maximum buffer size from the file
        bytesThisTime = fread(sendFileBuffer, 1, PROG3_BUFF_SIZE, file);
        if (bytesThisTime == 0) 
        {
            fprintf(stderr, "%s Error: read file: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(6);
        }
        bytesRead += bytesThisTime;

        //DEBUG PRINT
        printf("Read in %d bytes this time. bytesRead is %d and fileSize is %d\n", (int) bytesThisTime, (int) bytesRead, fileSize);

        //Send out the bytes read in this iteration to the client
        status = send(sock, sendFileBuffer, bytesThisTime, 0);
        if (status < 0) 
        {
            fprintf(stderr, "%s Error: send file: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(5);
        }
    }
    //DEBUG PRINT
    puts("Finished sending the file!");
}


/*
  Receives a file over the socket, one PROG3_BUFF_SIZE chunk at a time.

  sock: the socket to receive the file over
  file: the opened file you want to write to. Note: Does NOT reset file pointer!
  fileSize: the size (in bytes) of the file.
  programName: For error messages only.
       For example, if programName is "myProgram", this may print out 
      "myProgram Error: recv() file from server: timeout"
      -Required because both client (myftp) and server (myftpd) use this.
 */
void recvFile(int sock, FILE *f, unsigned int fileSize, 
              const char *programName)
{
    //Reads x number of bytes from server:
    char file[PROG3_BUFF_SIZE];
    int counter = 0;
    int fileRecvd;

    while(counter < fileSize){
        fileRecvd = recv(sock, file, PROG3_BUFF_SIZE, 0);
        if(fileRecvd < 0) 
        {
            fprintf(stderr, "%s Error: recv() file from server: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(3);
        } 
        fwrite(file, 1, fileRecvd, f);  //write into file f and save
        counter = counter + fileRecvd;
    }
}

/*
  Gets the file size of f, and resets f to point to the beginning of the file.
*/
unsigned int getFileSize(FILE *f)
{
    unsigned int toReturn;
    fseek(f, 0, SEEK_END);
    toReturn = (unsigned int) ftell(f);
    rewind(f);
    
    return toReturn;
}
