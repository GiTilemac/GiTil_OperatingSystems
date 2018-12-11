GAME SERVER IMPLEMENTATION USING MULTITHREADING

Author: Tilemachos S. Doganis

This project implements a game server, to which different players connect, request to reserve a portion of the server's resources, and can communicate with each other through a synchronous text messaging system.

The demo bash script "run_tests.sh" launches the server and three player threads using the Konsole application of the KDE Ubuntu distribution.

PROGRAM DESCRIPTION
The program is initialized by the 'main' function and is consisted of the following functions:

1. Game Function 'main()':
Runs in the background and creates new threads for the new matches. Initially one match thread is started, and when all required player spots are filled, an additional one is launched. Using a mutex and condition variables, it remains in a waiting state until it is updated by the change of the 'g_go' flag of the respective match, starting all the Player service Threads.

RUNTIME OPERATION
Reads and stores match parameters
Creates and prepares sockets for bind
Loop 1{
Create 1st thread for match
Locking using mutex and a condition variable until the current match starts
Reallocation of dynamic arrays of Match Threads and Match Start Flags 'g_go'
}

2. Match Function 'match()':
Launches when a new match starts
Runs in the background
Manages the players' connections to the server
Manages message forwarding through pipes, as well as their synchronization, using the Player service Threads

It shares the following variables with the Player service Threads, along with their respective mutexes:
Pipes for read/write from/to the Match Thread and the Player service Threads
Inventory, with each match's resources
Capacity, which counts the number of players connected to the match
Fl, which determines when a message from a player has been sent. No mutex is associated with this variable.
Synch, which counts the number of Player service Threads that have completed the waiting and messaging procedure that follows all players' connection, and are ready to proceed with the Messaging phase (phase mutex).

RUNTIME OPERATION
Creates Match variables
Prepares Player service Threads and mutexes
Starts listening for incoming connections
Enters Loop 1{
Receives new player connections
Prepares shared variables for the Player service Threads
Creates Player service Thread
Informs connected players on current match occupancy
}Exits Loop 1 & switches Match Start Flag & "wakes" all Player service Threads, when the necessary number of players is met
Enters Loop 2{
Waits for messages from Player service Thread
Forwards message to every Player service Thread
}


3. Player service function 'serve()':
Launches when a new player connects to the server
Manages the communication between server and the rest of the players
Forwards messages through pipes with the Match Thread.
Before match start it informs the player on the match occupancy every 5 seconds
After match start it transmits messages between the respective player and the server

RUNTIME OPERATION
Matches input variables of Match Thread with local pointers
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








