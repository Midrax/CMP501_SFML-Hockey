# CMP501 – Network Game Development

Assessment Report by Ridam Rahman
1804335@uad.ac.uk

---

**Architecture**

Given the requirements for this project the initial goal was to realize a simple multiplayer game meant for two players, i.e. a top down hockey game using SFML. 

After some research on the subject of which architecture was more suitable, ultimately the choice fell on the client-server architecture. The reason behind this choice was relatively dictated by the amount of prior knowledge and experience that was withheld on the subject, compared to what was known on peer-to-peer architectures. Last but not least, another important reason that was held into thought was how client-server architectures are commonly considered the best choice for action games, in comparison to peer-to-peer architectures (Fiedler, G., 2010). 

There are doubtlessly good reasons to choose one architecture or the other according to the situation, but at the end it was understood that peer-to-peer architectures are usually more suited for real-time strategy games where a certain degree of latency is not a heavy issue, or simple low-budget indie games, where a central server is not an affordable solution. This doesn’t imply that a peer-to-peer solution couldn’t have been used in this project, but there were other troublesome concerns with P2P architectures that required rethinking before opting for this solution: implementation difficulty is a well-known issue, as well as lag, and even cheating: since all the players are “equals”, the lack of an authoritative server would allow certain kinds of players to manipulate the data on their side to send false information to their peers. 

On the other hand, client-server architectures seem to avoid all these problems: compared to P2P they’re more straightforward implementation-wise, and are also easy to scale. With multiple clients, latency usually affects only one player as long as it doesn’t have a server role, and cheating can be detected and dealt with more easily thanks to the server’s authoritative power.

---

**Protocols**

The game has four different custom functions declared to handle communication on the application layer, but only two of them are used simultaneously depending on the role chosen by the player. Although they’re better described in the Integration section, their functionality can be easily summarized as “receiving” and “sending”, with small variations occurring between client and server implementations. All the player has to do is merely select what his role is between client and server when the application is launched.

The transport layer protocol that has been chosen for this application is TCP. Although there’s plenty of literature suggesting the use of UDP over TCP for action multiplayer games (Fiedler, G., 2008), TCP has been the first protocol to be considered for the scope of this project, due to some particular reasons. First of all, the magnitude of data we’re dealing with is considerably small, and the maximum amount of time latency that is found on non-cross-Atlantic connections is usually less than a 100ms, so both TCP and UDP would perform similarly (Ignatchenko, 2015). Since the game designed for this project doesn’t require the fast-paced high-level precision of shooting games, TCP has been considered a convenient choice.
The Transmission Control Protocol in fact requires less error handling than UDP, as it can easily deal with problems such as packet-loss, duplicate packages, or out-of-order packages, being built exactly to avoid these issues, and guarantee that the message sent is always received.

There are pros and cons to this choice: TCP makes sure that every packet sent is delivered, but this does cause a certain amount of overheard, and technically the most important packet that we need for the proper functioning of the game is the last one that has been sent. If given the last game state we were able to perfectly simulate the in-between passages, packet losses happening between certain intermediate steps could be ignored, thus making UDP the best choice. Nonetheless, this doesn’t stand true when certain critical data is sent, for example the change of direction ‘caused by the impact of the ball with one player. To avoid losing these important packets, it would be required to implement a packet-received acknowledgement system for critical data. As this would mean reinventing something we have already in TCP, due as well to a lack of time it was deemed better to use this protocol. 

---

**API** 

The API chosen to develop this project is the Simple and Fast Multimedia Library. It was considered suitable for the scope of this assignment as it is not only used commonly as a game building tool for small-scale indie projects, but does also feature a networking module that is easy to work with. Here are some of the classes from the module that were used for the game: 

-	sf::IpAddress
This class encapsulate an IPv4 network address. Among its features the most notable one is the support for multiple address representations. Although most of its potential was left unused in this project, a quick look at the SFML documentation allows us to see some particular properties, such as certain public fields like Localhost, or Broadcast, which allows to send the information over UDP to some special address that can share the data with every client connected to a network.

-	sf::Packet
This utility class can build blocks of data to transfer over the network, and provides a safe and easy way to serialize this data, as well as wrapping and unwrapping it, in order to send it over using sockets. Packets solve many problems that arise when transferring data, like BigEndian and LittleEndian conversion. 
 
-	sf::TcpListener
This class represents a listener socket, a special type of socket that is used in TCP connections, and listens to a given port and waits for connections on that port. When a new connection is received, the listener can accept and return a new instance of sf::TcpSocket that is properly initialized and can be used to communicate with the new client. Listener sockets are specific to the TCP protocol, since UDP sockets are connectionless and can therefore communicate directly.

-	sf::TcpSocket
This class is quite trivial, it simply represents a socket running over TCP.

---

**Integration**

Integrating the networking module with the rest of the application has been quite an easy process, since the entire game was built using the same library. As has been mentioned before, the networking process within the game is based on four functions, described here in more detail. These methods are: 

-	void TCPServerListener()
This method is launched on a separate thread, and is used by the server to listen for connections using the sf::TcpListener class from SFML. The only role of the listener class is to wait for incoming connection attempts on a given port, as it can't send or receive data. Once it has established a connection, the ServerListener thread enters a loop, within which the TCP socket can actively receive data from the connected client. If the received packet is not empty, the thread will lock the access to the resources in order to avoid any accidental overwriting by any other thread. It will then extrapolate the data inside the packet, after which the resource access is unlocked again. This loop will run until the window is open; if the window is closed the loop will end, and the thread running the function will be terminated.

-	void TCPClientListener()
This method is a variant of the aforementioned function meant for the client. It differs from the first one as it doesn’t use a Listener class; after all the client doesn’t need to wait for a connection attempt on any port, since it is the one attempting to connect: it is assumed that the client knows already the server’s IP address. 
This method is similar to the previous one only in relation to how the receiving loop works.

-	void TCPServerSend()
This method is used to send the game information from the server to the client. It locks the access to the packet in order to avoid any accidental packet overwriting, and sends the information over TCP before unlocking the resources.
The information sent from the server basically encodes the following information: the server-controlled player’s position and the disk’s position in relation to 2-dimensional space, and the disk’s angle.

-	void TCPClientSend()
As before, this is the client version of the previous method, and sends information about the client to the server. The other important difference from the server’s sending method is that the client doesn’t need to send any information about the disk’s location, or the disk’s angle, as the server is authoritative about this information.

While the Client and Server listener threads run separately, constantly waiting to receive packets, the send functions are called within the game’s main loop using a timer that is set to be called every third of a second. This is meant to avoid sending more data that it is needed, and have less overhead.

---

**Prediction**

The initial version of this game featured a “disk angle” that was randomly calculated at different moments of the game. These calculations happened at the very beginning of the game, and on player-collision with the disk. The problem that was met while setting up the network was that this “randomness” was being calculated both on the server and the client in separate ways, so the disk would often end up going in the wrong direction. To fix this behaviour, a few things had to be done. 

First of all, randomness had to be removed, so that connection drops wouldn’t affect the game engine so badly; the basic idea was to change the game’s rules and make them more deterministic, at least within a certain degree, so that even with a slight connection drop the disk’s correct position could still be extrapolated most of the time. 

The second thing that had to be fixed was who decided where the ball should go: to explain this a bit more, one should consider for example that if one player hits the disk with the leftmost part of its sprite, the disk should go towards the left, but if for some unforeseen reason the connection drops, one player may see the collision not happening, or the collision still happening, but with a different collision point. To fix this, the server had to be set as authoritative over the disk’s position, to curb the discrepancy, at least on one side.

At this point, only one problem was still left: determining and setting the players’ position in the fairest way possible, in order to avoid the aforementioned problem.
The simplest way one can think of to solve this is to constantly send the player’s position to the other end, and set the player’s position accordingly, but this, as can be easily guessed, may become heavily taxing on a network based on TCP. This solution may seem convenient at the beginning, on a local network, but as soon there’s some connection interference the players start to show a lagging effect. 

This final issue was solved through interpolation: code-wise speaking the adopted solution, given the current player position and the correct position sent by the other party, is to keep on moving the player by a small offset towards the correct position, until the difference between the two positions is zero. While this solution is pretty useful and convenient when we’re dealing with the players, using the same solution to move the disk if discrepancies are detected proved itself to be inefficient: there is no way to “naturally” change the disk’s direction in a way that makes it look normal; at the end, since the disk’s position is usually correctly extrapolated, in case of a huge connection drop the best solution is to reset the position of the disk, in order to resume the game without showing unnatural movements.

---

**Testing**

The game’s network has been tested by using Clumsy, a useful tool that can simulate lag, drops, out-of-order packets and other networking problems in an interactive manner. Given what has been written at the beginning of this report, the goal has been to be able to run the game smoothly with at least a 100ms lag, before trying to push the game towards further limits. During the tests a few issues were detected, depending on what was the simulated problem, and how large was its impact.

The first error that was simulated was latency. As long as the latency is below 300ms, the game is able to run quite smoothly, and the disk’s position is not affected much by the delay. But going towards higher latency makes the game unplayable for anyone except the server, since it is authoritative and decides where the disk is supposed to be. As has been said before, with a single-server multiple-client solution, the fairness of this choice becomes clearer, as lag affecting one client doesn’t affect all the other players, although in this case, since there are only two players, this decision may seem unbalancing.

The second error that was simulated was packet drops. In this case, the downsides of choosing TCP came out, as TCP tries always to make sure that all the packets are delivered, regardless of when they have been lost. Since previous game-states become useless in this game after a while, it is easy to imagine why choosing UDP would’ve mitigated the consequences of this error. This stands true until a certain point though: a 50% packet drop chance ruins the game regardless of the chosen transport protocol, so UDP’s most important advantage is the reduced traffic. Nonetheless, there are some errors where TCP outperforms UDP, at least in this game’s current implementation. 

Out of order packets for example in fact are always reordered correctly in TCP, thus making sure that the packets are always received in the correct order, while UDP needs to check extra information, such as when was the packet sent, before actually being able to update the current game state.

Duplicate packets are also dealt with in a quite fine way, thanks to TCP message acknowledgment system. UDP on the other hand would need to be able to check if the message has been received already, which would imply a small overhead on the receiver’s side.

The only problem that has been proven to be too complicated to solve was tampering, as verifying the packet content’s integrity and discarding the input has proved itself to be particularly complex; one solution might be to send some sort of checksum, and verify that this hasn’t been modified, but this hasn’t been tried in this project.

Demonstration video link:
https://youtu.be/0PyEjCGlO44

**References**

Fiedler, G. (2008). UDP vs. TCP | Gaffer On Games. [online] Gaffer On Games. Available at: https://gafferongames.com/post/udp_vs_tcp/ [Accessed 27 Nov. 2019].
Fiedler, G. (2010). What Every Programmer Needs To Know About Game Networking | Gaffer On Games. [online] Gaffer On Games. Available at: https://gafferongames.com/post/what_every_programmer_needs_to_know_about_game_networking/ [Accessed 26 Nov. 2019].
 Ignatchenko, S. (2015). Part IV: Great TCP-vs-UDP Debate of 64 Network DO’s and DONT’s for Game Engines - IT Hare on Soft.ware. [online] IT Hare on Soft.ware. Available at: http://ithare.com/64-network-dos-and-donts-for-game-engines-part-iv-great-tcp-vs-udp-debate/ [Accessed 27 Nov. 2019].
