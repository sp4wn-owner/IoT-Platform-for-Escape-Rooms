var WebSocketServer = require('ws').Server;

var wss = new WebSocketServer({port: 8080});

let users = {};
let devices = {};
let counter = 1;

wss.on('connection', function(connection) {
    console.log("device/user connected");

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
                    sendTo(connection, {
                        type: "connect",
                        success: false
                    });
                } else {
                    devices[data.name] = connection;
                    connection.name = data.name;
                    connection.type = 'device';
                    sendTo(connection, {
                        type: "connect",
                        success: true
                    });
                    addDevice(data.name, data.room);
                    console.log("device added: ", data.name);
                }
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
                    let deviceConn = devices[data.device];
                                    
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
            
            case 'toggleresult':
                let yourConn = users[data.name];

                if(checked === true) {                    
                    sendTo(yourConn, {
                        type: "toggle",                  
                        state: data.state,
                        device: data.devicename,
                        pin: data.pin,
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
                for(let i=0; i < devices.length; i++) {
                    dconn = devices[i];
                    sendTo(dconn, {
                        command: "OFF_ALL"
                    });    
                }
                delete users[connection.name];  
            } else if(connection.type == 'device') {
                console.log("On Close - deleting device", connection.name);
                delete devices[connection.name]; 
            }                    
         }        
     });  
});

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