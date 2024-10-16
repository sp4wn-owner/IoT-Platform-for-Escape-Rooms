const url = 'ws://10.0.0.211:4444';

let username;
let roomname;

let esp32pins = [2,4,5,12,14,15,18,19,21,22,23];

let loginpage = document.getElementById('login-page');
let usernameInput = document.getElementById('usernameinput')
let roomscontainer = document.getElementById('rooms-container');
let room1icon = document.getElementById('rm1-icon');
let room2icon = document.getElementById('rm2-icon');
let room3icon = document.getElementById('rm3-icon');
let room1 = document.getElementById('room1');
let room2 = document.getElementById('room2');
let room3 = document.getElementById('room3');
let devicescontainer = document.querySelectorAll('.devices-container');
let room1devices = document.getElementById('room1devices');
let room2devices = document.getElementById('room2devices');
let room3devices = document.getElementById('room3devices');
let deviceinfocontainer = document.querySelectorAll('.device-info-container');

let conn;
let reconnectAttempts = 0;
const maxReconnectAttempts = 5;
const reconnectDelay = 1000;

function connect() {
    conn = new WebSocket(url);

    conn.onopen = () => {
        console.log('Connected to the server');
        reconnectAttempts = 0;
    };

    conn.onmessage = function (msg) {
        console.log("Got message", msg.data);
     
        var data = JSON.parse(msg.data);
     
        switch(data.type) {
    
            case "login":
                handleLogin(data.success, data.name);
                break;
            
            case "devices":
                handleDevices(data.devices);
                break;
            
            case "toggle":
                handleToggle(data.state, data.id);
                break;         
           
            case "leave":
                handleLeave();
                break;
    
            default:
                break;
        }
     };

    conn.onerror = (error) => {
        console.error('WebSocket error:', error);
    };

    conn.onclose = () => {
        console.log('Connection closed, attempting to reconnect...');
        if (reconnectAttempts < maxReconnectAttempts) {
            reconnectAttempts++;
            setTimeout(connect, reconnectDelay * reconnectAttempts);
        } else {
            console.log('Max reconnect attempts reached. Please refresh the page.');
        }
    };
}

 function send(message) {
    conn.send(JSON.stringify(message));
 };

function init() {
    //loginpage.style.display = "none";
    //toggleroom('room1');
    connect();
    loginpage.style.display = "block";
    roomscontainer.style.display = "none";
    document.getElementsByTagName('header')[0].style.display = "none";
}

loginBtn.addEventListener("click", function (event) {
   
    username = usernameInput.value;
 
    if (username.length > 0) {
       send({
          type: "login",
          name: username
       });
    }
 });

function handleLogin(success, name) {
    if (success === false) {
       alert("Ooops...try a different username");
    } else {
       username = name;
       loginpage.style.display = "none";
       roomscontainer.style.display = "block";
       devicescontainer.innerHTML = "";
       toggleroom('room1');      
       document.getElementsByTagName('header')[0].style.display = "block";             
     }
};

function toggleroom(room) {
    switch (room) {
        case 'room1':
            room1icon.classList.add("active");
            room2icon.classList.remove("active");
            room3icon.classList.remove("active");
            room1.style.display = "block";  
            room2.style.display = "none";  
            room3.style.display = "none";            
            break;

        case 'room2':
            room1icon.classList.remove("active");
            room2icon.classList.add("active");
            room3icon.classList.remove("active");
            room1.style.display = "none";  
            room2.style.display = "block";  
            room3.style.display = "none";            
            break;

        case 'room3':
            room1icon.classList.remove("active");
            room2icon.classList.remove("active");
            room3icon.classList.add("active"); 
            room1.style.display = "none";  
            room2.style.display = "none";  
            room3.style.display = "block";           
            break;
    }
    roomname = room;
    getDevices(room);
}

function getDevices(room) {
     
    send({
        type: "getdevices",
        room: room
     });
}


function handleDevices(devices) {
    devicescontainer.forEach(devicescontainer => {
        devicescontainer.innerHTML = "";
     });
    const premadeDivTemplate = `
        <div class="device-info-container">
            <div class="form-group-devices">
                <span class="pin-number"></span>
                <div class="device-toggle-container">
                    <div class="toggle-container">
                        <input type="checkbox" class="toggle-button">
                        <label class="toggle-label"></label>
                    </div>
                </div>
            </div>                
        </div>`;
    
    let dcroom = document.getElementById(roomname + 'devices');
    if (!dcroom) {
        console.error('Target container not found.');
        return;
    }

    dcroom.innerHTML = '';

    for (let i = 0; i < devices.length; i++) {
        const divElement = document.createElement('div');
        divElement.classList.add('device-container');
        
        const divDeviceName = document.createElement('div');
        divDeviceName.classList.add('device-container-name');
        divDeviceName.innerHTML = devices[i].name;
        
        divElement.appendChild(divDeviceName);

        for (let j = 0; j < esp32pins.length; j++) {
            const tempContainer = document.createElement('div');
            tempContainer.innerHTML = premadeDivTemplate;

            const toggleButton = tempContainer.querySelector('.toggle-button');
            const toggleLabel = tempContainer.querySelector('.toggle-label');
            const pinSpan = tempContainer.querySelector('.pin-number');
            
            const pinNumber = esp32pins[j];
            pinSpan.innerHTML = `Pin ${pinNumber}`;
            
            const uniqueId = `toggle-button-${i}-${j}`;
            toggleButton.id = uniqueId;
            toggleLabel.setAttribute('for', uniqueId);

            toggleButton.setAttribute('data-device-name', devices[i].name);
            toggleButton.setAttribute('data-pin-number', pinNumber);

            toggleButton.addEventListener('change', function() {
                handleToggleChange(this);
            });

            divElement.appendChild(tempContainer);
        }
        
        dcroom.appendChild(divElement);
    }

    const spanElement = document.getElementById(roomname + "span");
    if (devices.length < 1) {
        spanElement.style.display = "block";
    } else {
        spanElement.style.display = "none";
    }
}

function handleToggleChange(toggle) {
    console.log(`Toggle button with ID ${toggle.id} changed to ${toggle.checked}`);
    let deviceName = toggle.getAttribute('data-device-name');
    let pin = toggle.getAttribute('data-pin-number');
    
    send({
        type: 'toggle',
        name: username,
        direction: "todevice",
        devicename: deviceName,
        pin: pin,
        id: toggle.id,
        state: toggle.checked
    });
}

function handleToggle(state, id) {
    if (state === false) {
        console.log("toggle off: " + id);
        setToggleState(id, false);
        console.log(id+": is off"); 
    } else if (state === true) {
        setToggleState(id, true);
       console.log(id+": is on");           
     }
};

function setToggleState(toggleId, state) {
    const toggleButton = document.getElementById(toggleId);
    if (toggleButton) {
        toggleButton.checked = state;
    }
}
//Might need to implement session storage (cookies) to handle client timeouts/re-login
init();