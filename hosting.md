## Hosting

### Community provided docker images:

- https://github.com/a-blondel/aemu-servers
- https://github.com/Naggioperso/docker-aemu-server

### Adhocctl server

Adhocctl server is used for group forming, peer discovery, and P2P routing. It functions as an adhocctl state tracker that makes the following data accessible from emulated adhocctl apis:

- Available groups during group scanning under certain game ID
- After joining a group, list of peers in the same group
- MAC address and P2P IP address for each peer (IP addresses is referenced by pspnet_adhoc through internal apis, to perform P2P networking)

As an extended feature, it also supports chat messages in group.

#### Deploying:

1. Download the server software, it's bundled with the client release https://github.com/Kethen/aemu/releases
2. Run the server software, for Windows x86_64, it's under `server_win/pspnet_adhocctl_server.exe`. For Linux x86_64, `cd dist_660_release/server; ./pspnet_adhocctl_server`
3. Open port TCP 27312 on your firewall
4. Port forward TCP 27312 on your internet gateway/router, if you are hosting on the internet

A nodejs based status page template is now also provided:

1. Install nodejs https://nodejs.org/en/download . On Linux, you can choose to use distro packaged nodejs instead, or use containerized solutions if you are familiar with setting that up
2. By default server listens to port 8080. If port 8080 is already in-use and you are not running the server in a container, you can find the port number at the top of `www/server.js` and change that
3. aemu_postoffice debug data url can be customized at the top of `www/server.js`, see below [Data relay](#data-relay) as to what aemu_postoffice is
4. Run `www/server.js` with nodejs
5. Open port TCP 8080 on your firewall
6. Port forward TCP 8080 / set up http proxy for the status page, if you are hosting on the internet


### Data relay

P2P networking can be hard to setup for clients due to ISP or router hardware restrictions. Therefore, a data relay is developed to make the process of connecting to other players easier, since users will no longer be required to setup port forwarding correctly. Reliability is not the same but similar to citra rooms. This is currently the default mode of operation of released clients, unless configured by user to use P2P mode instead.

Data relay is not perfect however, as all data go through the server, instead of requiring good connection between players as well as working forwarded ports on all clients, it'll require good network connection from server to all the clients.

#### Deploying:

1. Download the server software, it can be found here https://github.com/Kethen/aemu_postoffice/releases
2. Install nodejs https://nodejs.org/en/download . On Linux, you can choose to use distro packaged nodejs instead, or use containerized solutions if you are familiar with setting that up
3. Run `server_njs/aemu_postoffice.js` with nodejs
4. Open port TCP 27313 on your firewall
5. Port forward TCP 27313 on your internet gateway/router, if you are hosting on the internet

#### Note:

HTTP TCP 27314 provides a debug use json for inspecting the current state of the relay, the newly provided nodejs status page can parse the data from this debug json to show what ports a player is using for debugging connection issues between players.
