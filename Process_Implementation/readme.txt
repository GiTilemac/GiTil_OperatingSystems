GAME SERVER IMPLEMENTATION USING PROCESSES

Author: Tilemachos S. Doganis

This project implements a game server, to which different players connect, request to reserve a portion of the server's resources, and can communicate with each other through a synchronous text messaging system.

The demo bash script "run_tests.sh" launches the server and three player processes using the Konsole application of the KDE Ubuntu distribution.

PROGRAM DESCRIPTION
The program is initialized by the 'main' function and is consisted of the following processes:

1. Game Process:
Runs in the background and forks new matches. Initially one match is started, and when all required player spots are filled, an additional one is launched. The process is kept up-to-date using the 'mapipe' pipe, which connects it with the Match Process.
RUNTIME OPERATION
Reads and stores match parameters
Creates and prepares sockets for bind
1st fork for 1st match

2. Match Process:
Launches when a new match starts
Runs in the background
Manages the players' connections to the server
Manages message forwarding through pipes, using its children, as well as their synchronization
Its shared memory contains the following resources:
Inventory, with each match's resources
Capacity, which counts the number of players connected to the match
Go, which defines whether the match has started or not
Fl, which determines when a message from a player has been sent. The assumption is made that when a message from a player is sent, all others are ignored until it is forwarded to every other player
The code section responsible for Inventory and Capacity is a Critial Section, therefore the semaphore 'my_sem' is used to ensure consistent access and modification, in case of simultaneous attemps by multiple processes. The other two variables are flags, so no synchronization is necessary.

RUNTIME OPERATION
Creates shared memory and initializes semaphore
Starts listening for incoming connections
Enters Loop 1{
Receives new player connections
Forks Player service process
Informs connected players on current match occupancy
}Exits Loop 1 & begins match when the necessary number of players is met
Enters Loop 2{
Waits for messages from children
Forwards message to every child
}


3. Player service Process:
Launches when a new player connects to the server
Manages the communication between server and the rest of the players
Forwards messages through pipes with the Match Process.
Before match start it informs the player on the match occupancy every 5 seconds
After match start it transmits messages between the respective player and the server

RUNTIME OPERATION
Receives resource allocation request from player
Validity check and acceptance / rejection of resource allocation
If rejected, the process terminates, else:
Transmission of acceptance message to player
Receival of player name
Enters Loop 1{
Check for potential player disconnect
Receival of current match occupancy message from father pipe
Acceptance of 'OK' player message
}
Transmission of match start message to player
Clearing of any leftover 'OK' message from player
Enters Loop 2{
Check for potential player disconnect
Waits for message (from player -> send to parent / from parent pipe -> forward to player)








