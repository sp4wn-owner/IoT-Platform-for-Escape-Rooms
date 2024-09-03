var WebSocketServer = require('ws').Server;

var wss = new WebSocketServer({port: 4444});

let users = {};
let devices = {};
let counter = 1;

// Function to handle ping responses
const handlePing = (ws) => {
    ws.isAlive = true;
};

// Function to check clients' activity
const checkClients = () => {
    //console.log('Heartbeat check');
    wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
            console.log('Client terminated');
            return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping();
    });
};

// Set up a heartbeat to check clients every 5 seconds
const interval = setInterval(checkClients, 2000);

wss.on('connection', function(connection) {
    console.log("device/user connected");
    connection.isAlive = true;

    connection.on('message', function(message) {

        var data;
        try {
            data = JSON.parse(message);
        } catch (error) {
            console.log(error);
            data = {};
        }

        switch (data.type) {

            case 'login':
                console.log("user connected", data.name);
                if(users[data.name]) {
                    sendTo(connection, {
                        type: "login",
                        success: false
                    });
                } else {
                    users[data.name] = connection;
                    connection.name = data.name;
                    connection.type = 'user';              

                    sendTo(connection, {
                        type: "login",
                        success: true,
                        name: data.name
                    });
                }
                break;

            case 'connect':
                console.log("device connected", data.name);
                if(devices[data.name]) {
                    devices[data.name] = connection;
                    connection.name = data.name;
                    connection.rooom = data.room;
                    connection.type = 'device';
                    
                } else {
                    devices[data.name] = connection;
                    connection.name = data.name;
                    connection.type = 'device';
                    connection.rooom = data.room;
                    addDevice(data.name, data.room);
                    console.log("device added: ", data.name);
                }

                sendDevices(data.room);
                
                break;

            case 'getdevices':
                let devicesByRoom = getDevicesByFieldValue(devices, "room", data.room);
                
                sendTo(connection, {
                    type: "devices",                  
                    devices: devicesByRoom
                });

                break;
            
            case 'toggle':
                if(data.direction == "todevice") {
                    let deviceConn = devices[data.devicename];
                                    
                    sendTo(deviceConn, {
                        type: "toggle", 
                        name: data.name,                 
                        state: data.state,
                        device: data.devicename,
                        pin: data.pin,
                        id: data.id
                    });
                } else if(data.direction == "touser") {
                    let yourConn = users[data.name];
                                   
                    sendTo(yourConn, {
                        type: "toggle",                
                        state: data.state,
                        id: data.id
                    });
                }
                  
                break;    
                 
        }
    });

    connection.on("close", function() {
        if(connection.name) {
            if(connection.type == 'user') {
                console.log("On Close - deleting user: ", connection.name);
                // Convert object to array of device objects
                let devicesArray = Object.keys(devices).map(key => ({
                    name: key,
                    ...devices[key]
                }));
                devicesArray.forEach(device => {
                    let dconn = devices[device.name];  // Set dconn to the name of the device
                    sendTo(dconn, {
                        command: "OFF_ALL"
                    });    
                  });
                
                delete users[connection.name];  
            } if(connection.type == 'device') {
                console.log("On Close - deleting device", connection.name);
                
                let usersArray = Object.keys(users).map(key => ({
                    name: key,
                    ...users[key]
                }));
                usersArray.forEach(user => {
                    let userconn = users[user.name];  // Set conn to the name of the user

                    for (let deviceKey in devices) {
                        let dconn = devices[deviceKey];
                        if (dconn.name === connection.name) {
                            //let a = dconn.room;
                            let devicesByRoom = getDevicesByFieldValue(devices, "room", connection.rooom);

                            // Optionally send a command to the device    
                            sendTo(userconn, {
                                type: "devices",                  
                                devices: devicesByRoom
                            });                       
                            delete devices[deviceKey]; // Delete the device from the devices object
                        }
                    }                    
                
                });
                // Remove all items related to this device
                
                delete devices[connection.name]; 
            } 
           // clearInterval(interval);                   
         }        
     });

     connection.on('pong', () => handlePing(connection));
});

// Log server start
console.log('WebSocket server started on ws://localhost:4444');

function sendTo(connection, message) {
    connection.send(JSON.stringify(message));
}

function getDevicesByFieldValue(devices, fieldName, fieldValue) {
    let matchingDevices = [];

    for (let deviceKey in devices) {
        if (devices[deviceKey][fieldName] === fieldValue) {
            matchingDevices.push(devices[deviceKey]);
        }
    }

    return matchingDevices;
}

function addDevice(name, room) {
    let key = counter; 
    devices[key] = {             
        name: name,
        room: room,
        deviceID: key
    };
    counter++; 
}

function sendDevices(room) {
    let usersArray = Object.keys(users).map(key => ({
        name: key,
        ...users[key]
    }));
    usersArray.forEach(user => {
        let userconn = users[user.name];  // Set conn to the name of the user

        //let a = dconn.room;
        let devicesByRoom = getDevicesByFieldValue(devices, "room", room);

        // Optionally send a command to the device    
        sendTo(userconn, {
            type: "devices",                  
            devices: devicesByRoom
        });                  
    });
}
