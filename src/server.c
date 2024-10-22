#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8082
#define BUFFER_SIZE 256
#define MAX_USERS 100
typedef struct {
    char user_id[20];
    char username[50];
    char password[100];
    char phone_no[11];
    int is_registered;

} UserRegistration;

typedef struct {
    char username[50];
    int is_forwarding_active;
    char forwarding_type[20];
    char phone_no[11];
    char destination_number[11];
    int is_busy;  // Indicates if the user is busy
} UserForwarding;

typedef struct{
        char caller[11];
      char timestamp[50];
}callLog;



UserRegistration users[MAX_USERS];
UserForwarding userForwardings[MAX_USERS];
callLog callLogs[MAX_USERS];
int userCount = 0;
int forwardingCount = 0;
int callCount = 0;

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

/*Loads user data from users.txt into the users array.
-Opens the users.txt file in read mode.
-Reads each user's information (ID, username, password, phone number, and registration status).
-Increments userCount after adding a new user.
-Prints an error message if the file fails to open or close.a
*/

void loadUsersFromFile() {
    FILE *file = fopen("data/users.txt", "r");
    if (!file){
                perror("Error opening users.txt");
                return;}

    while (userCount<MAX_USERS && fscanf(file, "%[^,],%[^,],%[^,],%[^,],%d\n",
                  users[userCount].user_id,
                  users[userCount].username,
                  users[userCount].password,
                  users[userCount].phone_no,
                  &users[userCount].is_registered) == 5 ) {
        userCount++;
    }
    if(fclose(file)!=0){
                perror("Error closing user.txt");
        }
}

/*Loads call forwarding data from forwardings.txt into the userForwardings array.
-Opens forwardings.txt in read mode.
-Reads forwarding data (username, type, destination number, etc.).
-Increments forwardingCount after processing each entry.
-Handles file open/close errors with error messages.
*/

void loadForwardingsFromFile() {
    FILE *file = fopen("data/forwardings.txt", "r");
    if (!file){
                perror("Error opening forwardings.txt");
                return;
        }

    while (forwardingCount<MAX_USERS && fscanf(file, "%[^,],%d,%[^,],%s,%s,%d\n",
                  userForwardings[forwardingCount].username,
                  &userForwardings[forwardingCount].is_forwarding_active,
                  userForwardings[forwardingCount].forwarding_type,
 userForwardings[forwardingCount].phone_no,
                  userForwardings[forwardingCount].destination_number,
                  &userForwardings[forwardingCount].is_busy) == 6) {
        forwardingCount++;
    }
    if(fclose(file)!=0){
                perror("Error closing forwardings.txt");
        }
}

/* Loads call log entries from call_log.txt into the callLogs array.
-Opens call_log.txt in read mode.
-Reads each log's caller and timestamp.
-Increments callCount for each entry.
-Prints errors if the file fails to open or close.
*/

void loadcall(){
        FILE *file = fopen("data/call_log.txt","r");
        if(!file) {
                perror("Error opening forwardings.txt");
                return;
        }
        while(callCount<MAX_USERS && fscanf(file, "%[^,],%[^,]\n",
                     callLogs[callCount].caller,
                     callLogs[callCount].timestamp)==2){
              callCount++;
}
    if( fclose(file)!= 0){
                perror("Error closing forwardings.txt");

        }
}


/*Saves the current user data into users.txt
-Opens users.txt in write mode.
-Writes each user’s details to the file.
-Handles file opening and closing errors.
*/

void saveUsersToFile() {
    FILE *file = fopen("data/users.txt", "w");
        if(!file){
                perror("Error opening users.txt for writing");
                return;
 }
    for (int i = 0; i < userCount; i++) {
        fprintf(file, "%s,%s,%s,%s,%d\n",
                users[i].user_id,
                users[i].username,
                users[i].password,
                users[i].phone_no,
                users[i].is_registered);

    }
    if(fclose(file)!=0){
                perror("Error closing users.txt");

        }
}

/*Saves call forwarding data into forwardings.txt.
-Opens forwardings.txt in write mode.
-Writes each forwarding entry to the file.
-Handles file-related errors appropriately.
*/

void saveForwardingsToFile() {
    FILE *file = fopen("data/forwardings.txt", "a");
        if (!file) {
        perror("Error opening forwardings.txt for writing");
        return;
    }
    for (int i = 0; i < forwardingCount; i++) {
        fprintf(file, "%s,%d,%s,%s,%s,%d\n",
                userForwardings[i].username,
                userForwardings[i].is_forwarding_active,
                userForwardings[i].forwarding_type,
                userForwardings[i].phone_no,
                userForwardings[i].destination_number,
                userForwardings[i].is_busy);
    }
   if( fclose(file) != 0){
                perror("Error closing forwardings.txt");
}
}

/*Appends a new call log entry with the caller and timestamp to call_log.txt.
-Records the current time.
-Logs the call to call_log.txt.
-Handles file errors during logging.
*/

void logCall(const char *caller) {
    FILE *file = fopen("data/call_log.txt", "a");
    if (!file) {
                perror("Error opening call_log.txt for appending");
                return;
        }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0; // Remove newline character from timestamp

    fprintf(file, "Caller: %s, Timestamp: %s\n", caller, timestamp);
   if( fclose(file) != 0){
                perror("error closing call_log.txt");

   }
}

/*Registers a new user and initializes their forwarding settings.
-Registers a new user if the limit is not reached.
-Initializes call forwarding for the user.
-Saves the user and forwarding information to files.
-Sends appropriate responses to the client.
*/

void registerUser(const char *username, const char *password, char *phone_no, int client_socket) {
    pthread_mutex_lock(&user_mutex);
    if (userCount >= MAX_USERS) {
        send(client_socket, "User limit reached.\n", BUFFER_SIZE, 0);
        pthread_mutex_unlock(&user_mutex);
        return;
    }
    for(int i=0;i<userCount;i++){
        if(strcmp(users[i].username,username)==0 || strcmp(users[i].phone_no,phone_no)==0){
                send(client_socket,"User already registered\n",BUFFER_SIZE,0);
                pthread_mutex_unlock(&user_mutex);
                return;
}       }
sprintf(users[userCount].user_id, "U%d", userCount + 1);
    strcpy(users[userCount].username, username);
    strcpy(users[userCount].password, password);
    strcpy(users[userCount].phone_no, phone_no);
    users[userCount].is_registered = 1;

        userCount++;


   // initializing forwarding information
    strcpy(userForwardings[forwardingCount].username, username);
    userForwardings[forwardingCount].is_forwarding_active = 0;
    strcpy(userForwardings[forwardingCount].forwarding_type, "");
    strcpy(userForwardings[forwardingCount].phone_no,phone_no);
    strcpy(userForwardings[forwardingCount].destination_number, "");
    userForwardings[forwardingCount].is_busy = 0;
    forwardingCount++;

    saveUsersToFile();
        send(client_socket, "User registered successfully. \n", BUFFER_SIZE, 0);

    saveForwardingsToFile();
    pthread_mutex_unlock(&user_mutex);
}


/*Activates call forwarding for a user with a specific type.
-Activates forwarding if the user exists and type is valid.
-Updates the forwarding settings in the file.
-Sends success or error messages to the client.
*/

void activateCallForwarding(const char *username, const char *type, const char *phone_no, const char *destination, int client_socket) {
    pthread_mutex_lock(&user_mutex);

    for (int i = 0; i < forwardingCount; i++) {
        if (strcmp(userForwardings[i].username, username) == 0 && strcmp(userForwardings[i].phone_no,phone_no)==0) {
            userForwardings[i].is_forwarding_active = 1;
                if(strcmp(type,"Busy")==0||strcmp(type,"Unanswered")==0||strcmp(type,"Unconditional")==0){
            strcpy(userForwardings[i].forwarding_type, type);}
                else{
 send(client_socket,"enter a valid call forwarding type\n",BUFFER_SIZE,0);
                   pthread_mutex_unlock(&user_mutex);
            }
            strcpy(userForwardings[i].phone_no,phone_no);
            strcpy(userForwardings[i].destination_number, destination);
            saveForwardingsToFile();
            send(client_socket, "Call forwarding activated.\n", BUFFER_SIZE, 0);
            pthread_mutex_unlock(&user_mutex);
            return;
        }
    }
    send(client_socket, "User not found or not registered.\n", BUFFER_SIZE, 0);
    pthread_mutex_unlock(&user_mutex);
}

/*Deactivates call forwarding for a user.
-Deactivates forwarding if the user is found.
-Saves updated settings to the file.
-Notifies the client about the operation status.
*/

void deactivateCallForwarding(const char *username, const char *phone_no, int client_socket) {
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < forwardingCount; i++) {
        if (strcmp(userForwardings[i].username, username) == 0 && strcmp(userForwardings[i].phone_no,phone_no)==0 && userForwardings[i].is_forwarding_active==1) {
            userForwardings[i].is_forwarding_active = 0;
            saveForwardingsToFile();
            send(client_socket, "Call forwarding deactivated.\n", BUFFER_SIZE, 0);
            pthread_mutex_unlock(&user_mutex);
            return;
        }
    }
    send(client_socket, "User not found or not registered.\n", BUFFER_SIZE, 0);
    pthread_mutex_unlock(&user_mutex);
}

/*Authenticates a user using their username, password, and phone number.
-Verifies the user credentials.
-Sends authentication status to the client.
*/

void authenticateUser(const char *username,const char *phone_no, const char *password, int client_socket) {
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0 && strcmp(users[i].phone_no,phone_no)==0) {
            send(client_socket, "Authentication successful.\n", BUFFER_SIZE, 0);
            pthread_mutex_unlock(&user_mutex);
            return;
        }
    }
    send(client_socket, "Authentication failed.\n", BUFFER_SIZE, 0);
    pthread_mutex_unlock(&user_mutex);
}

/*Allows a user to change their password.
-Updates the password if the current credentials are valid.
-Saves the updated information to the file.
-Notifies the client about success or failure.
*/

void changePassword(  char *phone_no,const char *password, const char *new_password, int client_socket ){
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < userCount; i++) {
    if (strcmp(users[i].password, password) == 0 && strcmp(users[i].phone_no,phone_no)==0) {
        strcpy(users[i].password, new_password);
        saveUsersToFile();
        send(client_socket, "Password changed successfully.\n", BUFFER_SIZE, 0);
                pthread_mutex_unlock(&user_mutex);
        return;
       }
}
   send(client_socket, "username not found.\n",BUFFER_SIZE,0);
   pthread_mutex_unlock(&user_mutex);
   }

/*Displays the call log entries for a specific caller.
-Searches for call logs related to the caller.
-Sends the log details to the client.
*/
void displayCallLog(char *caller, int client_socket){
        pthread_mutex_lock(&user_mutex);
        char buffer[BUFFER_SIZE];
        for(int i=0;i<callCount;i++){
        if(strcmp(callLogs[i].caller,caller)==0){
                printf(buffer, BUFFER_SIZE, "%s\n", callLogs[i].timestamp);
        }
        send(client_socket,buffer,BUFFER_SIZE,0);
        pthread_mutex_unlock(&user_mutex);
        return;
        }
        send(client_socket,"caller not found.\n",BUFFER_SIZE,0);
        pthread_mutex_unlock(&user_mutex);

}

/*Unregisters a user by setting their is_registered flag to 0.
-Removes the user if credentials are correct.
-Saves the updated user list to the file.
-Sends a success or failure message to the client.
*/

void unregisterUser(const char *phone_no, const char *password, int client_socket){
        for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].phone_no, phone_no) == 0 && strcmp(users[i].password,password)==0 && users[i].is_registered==1) {
        users[i].is_registered = 0; //set to zero if unregistered, 1 if registered
        saveUsersToFile();
                send(client_socket, "User unregistered successfully. \n", BUFFER_SIZE, 0);
        pthread_mutex_unlock(&user_mutex);
                return;

        }

        }

    send(client_socket, "No such user exists. \n", BUFFER_SIZE, 0);
    pthread_mutex_unlock(&user_mutex);
}

 /*Manages call handling based on forwarding rules.
-Checks if the callee has call forwarding enabled.
-Handles forwarding based on the type (busy, unanswered, unconditional).
-Logs the call if needed.
-Sends appropriate messages to the client.
*/

void handleCall(const char *caller, const char *callee, const char *phone_no, int client_socket) {
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < forwardingCount; i++) {

        if(strcmp(caller,userForwardings[i].destination_number)==0){
                        send(client_socket,"caller and destination number are same\n",BUFFER_SIZE,0);
                        pthread_mutex_unlock(&user_mutex);
						return;
        }
    if(strcmp(phone_no,caller)==0){
                send(client_socket,"caller and callee number are same",BUFFER_SIZE,0);
                pthread_mutex_unlock(&user_mutex);
				return;
        }
        if (strcmp(userForwardings[i].username, callee) == 0 && strcmp(userForwardings[i].phone_no,phone_no)==0 && userForwardings[i].is_forwarding_active == 1) {

            logCall(caller);  // Log the call

            if (strcmp(userForwardings[i].forwarding_type,"Busy")==0 && userForwardings[i].is_busy==1 &&  userForwardings[i].is_forwarding_active == 1) {
                char response[BUFFER_SIZE];
                sprintf(response, "Call from %s is forwarded to %s.\n", caller, userForwardings[i].destination_number);
                send(client_socket, response, BUFFER_SIZE, 0);
                userForwardings[i].is_busy = 1;  // Reset busy status after call handling
                pthread_mutex_unlock(&user_mutex);
                return;
            }
          else if(strcmp(userForwardings[i].forwarding_type,"Unanswered")==0 &&  userForwardings[i].is_forwarding_active == 1){
                char response[BUFFER_SIZE];
                        sleep(10);
                sprintf(response, "Call from %s is forwarded to %s.\n", caller, userForwardings[i].destination_number);
                send(client_socket, response, BUFFER_SIZE, 0);
                pthread_mutex_unlock(&user_mutex);
                return;
}
         else if(strcmp(userForwardings[i].forwarding_type,"Unconditional")==0 &&  userForwardings[i].is_forwarding_active == 1){
                char response[BUFFER_SIZE];
                sprintf(response, "Call from %s is forwarded to %s.\n", caller, userForwardings[i].destination_number);
                send(client_socket, response, BUFFER_SIZE, 0);
                pthread_mutex_unlock(&user_mutex);
                return;
}

          else {
                send(client_socket, "Call connected normally.\n", BUFFER_SIZE, 0);
                userForwardings[i].is_busy = 1;  // Reset busy status after call handling
                pthread_mutex_unlock(&user_mutex);
                return;
            }
        }
                else if(strcmp(userForwardings[i].username, callee) == 0 && strcmp(userForwardings[i].phone_no,phone_no)==0){
                        send(client_socket, "Call connected normally.\n", BUFFER_SIZE, 0);
                    pthread_mutex_unlock(&user_mutex);
                    return;
                }
    }
    send(client_socket, "Callee not found.\n", BUFFER_SIZE, 0);
    pthread_mutex_unlock(&user_mutex);
}

/*Handles client commands by reading from the socket.
-Reads commands from the client and invokes corresponding functions.
-Sends responses to the client based on the command’s success.
*/

void *clientHandler(void *socket_desc) {
    int client_socket = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int valread;

    while ((valread = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        buffer[valread] = '\0';
        char command[20], username[50], password[100], forwardingType[20], destination[11], callee[50], phone_no[11], new_password[100], caller[11];
// Command parsing with more flexibility
        if (sscanf(buffer, "%19s", command) == 1) {
            if (strcmp(command, "REGISTER") == 0 && sscanf(buffer, "%*s %49s %99s %10s", username, password, phone_no) == 3) {
                registerUser(username, password, phone_no, client_socket);
            } else if (strcmp(command, "LOGIN") == 0 && sscanf(buffer, "%*s %49s %10s %99s", username, phone_no, password) == 3) {
                authenticateUser(username, phone_no, password, client_socket);
            } else if (strcmp(command, "ACTIVATE") == 0 && sscanf(buffer, "%*s %49s %19s %10s %10s", username, forwardingType, phone_no, destination) == 4) {
                activateCallForwarding(username, forwardingType, phone_no, destination, client_socket);
            } else if (strcmp(command, "DEACTIVATE") == 0 && sscanf(buffer, "%*s %49s %10s", username, phone_no) == 2) {
                deactivateCallForwarding(username, phone_no, client_socket);
            } else if (strcmp(command, "CALL") == 0 && sscanf(buffer, "%*s %49s %49s %10s", username, callee, phone_no) == 3) {
                handleCall(username, callee, phone_no, client_socket);
                        } else if (strcmp(command, "CHANGE_PASSWORD")==0 && sscanf(buffer, "%*s %10s %99s %99s",phone_no,password,new_password)==3){
                            changePassword(phone_no,password,new_password,client_socket);
                        }
                        else if(strcmp(command, "CALLLOG")==0 && sscanf(buffer, "%*s %10s",caller)==1){
                                displayCallLog(caller,client_socket);
                        }
                        else if (strcmp(command, "UNREGISTER") == 0 && sscanf(buffer, "%*s %10s %49s", phone_no, password) == 2) {
                unregisterUser(phone_no, password,  client_socket);
                        }
                        else {
                send(client_socket, "Invalid command or parameters.\n", BUFFER_SIZE, 0);
            }
        }
    }

   if( close(client_socket) != 0){
                perror("Error closing client socket");
   }
    free(socket_desc);
 return NULL;
}

/*Entry point of the program that sets up the server.
-Creates the server socket and binds it to the specified port.
-Listens for incoming connections.
-Spawns a new thread for each client connection.
-Loads user, forwarding, and call log data from files.
-Cleans up resources upon termination.
*/

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t thread_id;


   //create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }


    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }


    // Start listening for connections
    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }


 printf("Server listening on port %d\n", PORT);
    loadUsersFromFile();
        loadForwardingsFromFile();
        loadcall();

    while (1) {
        // Accept a new connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connection accepted\n");

        // Create a new thread for the client
        int *new_sock = malloc(1);
        *new_sock = client_socket;
        if (pthread_create(&thread_id, NULL, clientHandler, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
                        close(client_socket);
            continue;
        }

        // Detach the thread
       if( pthread_detach(thread_id)!=0){
                        perror("could not detach thread");
                        close(client_socket);
                        free(new_sock);
           }
    }

    // Cleanup
   if( close(server_socket)!=0){
                perror("Error closing server socket");
   }
    return 0;
}
