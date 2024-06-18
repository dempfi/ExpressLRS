var Joy1;
var Joy2;
var Slider1;
var Slider2;

var shrew_task_timer;
var mixer_dirty = false;
var mixer_prev = "";
var wakeLock = null;

const CRSF_CHANNEL_VALUE_MIN  = 172;
const CRSF_CHANNEL_VALUE_1000 = 191;
const CRSF_CHANNEL_VALUE_MID  = 992;
const CRSF_CHANNEL_VALUE_2000 = 1792;
const CRSF_CHANNEL_VALUE_MAX  = 1811;

const channel = new Array(16).fill(CRSF_CHANNEL_VALUE_MID);
const channel16 = new Uint16Array(16);
const variable = new Array(32).fill(0);

function shrew_onLoad() {
    Joy1 = new JoyStick('joy1Div');
    Joy2 = new JoyStick('joy2Div');
    Slider1 = document.getElementById("slider_1");
    Slider2 = document.getElementById("slider_2");
    document.getElementById('joystick_area').classList.add("hidden");
    createDebugGrid();
    configLoad();
    websock_init();
    setupGamepadEvents();
    const requestWakeLock = async () => {
        try {
            wakeLock = await navigator.wakeLock.request('screen');
            console.log('Screen wake lock is active');
            wakeLock.addEventListener('release', () => {
                console.log('Screen wake lock was released');
            });
        } catch (err) {
            console.error(`${err.name}, ${err.message}`);
        }
    };
    document.addEventListener('visibilitychange', async () => {
        if (wakeLock !== null && document.visibilityState === 'visible') {
            await requestWakeLock();
        }
    });
    requestWakeLock();
    shrew_task_timer = requestAnimationFrame(shrew_task);
}

function shrew_task()
{
    updateMixerFunction();
    if (MyGamepad == null) {
        updateGamepadState();
    } else {
        pollGamepad();
    }
    var tosend = false;
    try {
        var contents = "function mixer_run(){\r\n";
        contents += mixer_prev;
        contents += "\r\n}";
        eval(contents);
        if (typeof mixer_run === 'function') {
            tosend = mixer_run();
        }
    } catch (e) {
        console.error('error running mixer:', e);
    }
    fillDebugCells();
    if (ws_checkConnection())
    {
        if (ws.bufferedAmount === 0)
        {
            if (typeof tosend === 'boolean' && tosend === true) {
                for (let i = 0; i < channel.length; i++) {
                    var x = clamp(channel[i], 0, 2048);
                    channel16[i] = Math.round(x);
                }
                var buffer = new ArrayBuffer(channel.length * 2);
                var dataview = new DataView(buffer);
                channel16.forEach(function(x, idx) {
                    dataview.setUint16(idx * 2, x, true); // true for little-endian
                });
                ws.send(buffer);
            }
        }
    }
    shrew_task_timer = requestAnimationFrame(shrew_task);
}

function updateMixerFunction() {
    if (mixer_dirty)
    {
        mixer_dirty = false;
        var secondColumn = document.getElementById('second_column');
        var touchArea = document.getElementById('joystick_area');
        var gamepadArea = document.getElementById('gamepad_area');
        if (mixer_prev.includes("MyGamepad")) {
            gamepadArea.classList.remove('hidden');
        }
        else {
            gamepadArea.classList.add('hidden');
        }
        if (mixer_prev.includes("Joy1") || mixer_prev.includes("Joy2")) {
            touchArea.classList.remove('hidden');
        }
        else {
            touchArea.classList.add('hidden');
        }
        if (mixer_prev.includes("Joy2")) {
            secondColumn.classList.remove('hidden');
        }
        else {
            secondColumn.classList.add('hidden');
        }
    }
}

var ws;
var ws_reconnect_timer = null;
var ws_timestamp = Date.now();
function websock_init() {
    const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`${protocol}://${window.location.host}/shrew_ws`);
    var timeout = setTimeout(function() {
        if (ws.readyState !== WebSocket.OPEN) {
            console.log('Connection timeout. Closing WebSocket.');
            ws.close();
            ws_primeReconnect();
        }
    }, 3000);
    ws.onopen = function() {
        clearTimeout(timeout);
        console.log('WebSocket connection established');
    };
    ws.onmessage = function(event) {
        ws_timestamp = Date.now();
        console.log('WebSocket data received:', event.data);
    };
    ws.onclose = function(event) {
        console.log('WebSocket closed, event reason:', event.reason);
        ws_checkConnection();
    };
    ws.onerror = function(error) {
        console.error('WebSocket error:', error);
        ws_checkConnection();
    };
}

function ws_primeReconnect() {
    clearTimeout(ws_reconnect_timer);
    ws_reconnect_timer = setTimeout(function() {
        websock_init();
    }, 1000);
}

function ws_checkConnection() {
    if (ws.readyState !== WebSocket.OPEN) {
        console.log('WebSocket connection lost');
        ws.close();
        ws_primeReconnect();
        return false;
    }
    else {
        var currentTime = Date.now();
        var timedout = ((currentTime - ws_timestamp) >= 1000);
        if (timedout) {
            console.log('WebSocket no response, closing');
            ws.close();
            ws_primeReconnect();
            return false;
        }
    }
    return true;
}

function configLoad() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "shrew_cfgload", true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            if (xhr.status === 200) {
                try {
                    var jsonData = JSON.parse(xhr.responseText);
                    populateFormWithData(jsonData);
                } catch (e) {
                    console.error('Could not parse JSON data:', e);
                }
            } else {
                console.error('Request failed with status:', xhr.status);
                setTimeout(configLoad, 1000);
            }
        }
    };
}

function configSave() {
  var formData = new FormData(document.getElementById('config_data_form'));
  let object = {};
  formData.forEach((value, key) => {
    if(object.hasOwnProperty(key)) {
      if(Array.isArray(object[key])) {
        object[key].push(value);
      } else {
        object[key] = [object[key], value];
      }
    } else {
      object[key] = value;
    }
  });
  let formJson = JSON.stringify(object);
  var xhr = new XMLHttpRequest();
  xhr.open('POST', 'shrew_cfgsave', true);
  xhr.onload = function () {
    if (xhr.status === 200) {
      console.log('btn_configSave: form submitted successfully');
    } else {
      console.error('btn_configSave: an error occurred!');
    }
  };
  xhr.send(formJson);
}

function populateFormWithData(jsonData) {
    for (var key in jsonData) {
        if (jsonData.hasOwnProperty(key)) {
            var element = document.getElementById(key);
            if (element) {
                if (element.type === 'checkbox') {
                    element.checked = jsonData[key];
                } else {
                    element.value = jsonData[key];
                }
            }
        }
    }
    mixer_onChange();
}

function mixer_onChange() {
    var txtele = document.getElementById('txt_mixer');
    var newtxt = txtele.value;
    if (newtxt != mixer_prev) {
        mixer_dirty = true;
        mixer_prev = newtxt;
    }
}

function showHideConfig() {
    var div = document.getElementById('div_config');
    div.classList.toggle('hidden');
}

function showHideDebug() {
    var div = document.getElementById('debug_hider');
    div.classList.toggle('hidden');
}

function clamp(value, limit1, limit2) {
    var min = Math.min(limit1, limit2);
    var max = Math.max(limit1, limit2);
    return Math.min(Math.max(value, min), max);
}

function mapRange(value, low1, high1, low2, high2, limit) {
    var x = low2 + (high2 - low2) * (value - low1) / (high1 - low1);
    if (limit) {
        x = clamp(x, low2, high2);
    }
    return x;
}

var activeGamepadIndex = null;
var activeGamepadId = null;
var MyGamepad = null;

function updateGamepadState() {
    const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
    for (let i = 0; i < gamepads.length; i++) {
        const gamepad = gamepads[i];
        if (gamepad && activeGamepadIndex === null) {
            if (gamepad.buttons.some(button => button.pressed) || gamepad.axes.some(axis => Math.abs(axis) > 0.1)) {
                activeGamepadIndex = gamepad.index;
                console.log(`Active gamepad set to index ${activeGamepadIndex}`);
                MyGamepad = navigator.getGamepads()[activeGamepadIndex];
                if (activeGamepadId != MyGamepad.id) {
                    activeGamepadId = MyGamepad.id;
                    buildGamepadView();
                }
            }
        }
    }
}

function buildGamepadView() {
    document.getElementById('gamepad_id').innerHTML = "Gamepad: " + MyGamepad.id;
    var container = document.getElementById('gamepad_buttons');
    container.innerHTML = "";
    for (let i = 0; i < MyGamepad.buttons.length; i++) {
        const square = document.createElement('span');
        square.classList.add('square');
        square.id = "sqr_btn_" + i;
        square.innerHTML = i.toString();
        container.appendChild(square);
    }
    container = document.getElementById('gamepad_axis');
    container.innerHTML = "";
    for (let i = 0; i < MyGamepad.axes.length; i++) {
        const square = document.createElement('span');
        square.classList.add('square');
        square.id = "sqr_axes_" + i;
        square.innerHTML = i.toString();
        container.appendChild(square);
    }
    pollGamepad();
}

function pollGamepad() {
    const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
    MyGamepad = gamepads[activeGamepadIndex];
    for (let i = 0; i < MyGamepad.buttons.length; i++) {
        var ele = document.getElementById("sqr_btn_" + i);
        if (MyGamepad.buttons[i].pressed) {
            var r = 127; var g = 127; var b = 127; var x = MyGamepad.buttons[i].value;
            r = mapRange(x, 0, 1, 127, 255, true);
            g = mapRange(x, 0, 1, 127, 0, true);
            b = g;
            ele.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
            ele.style.borderColor     = 'green';
        }
        else {
            ele.style.backgroundColor = 'gray';
            ele.style.borderColor     = 'black';
        }
    }
    for (let i = 0; i < MyGamepad.axes.length; i++) {
        var ele = document.getElementById("sqr_axes_" + i);
        var x = MyGamepad.axes[i];
        var r = 0; var g = 0; var b = 0;
        if (x < 0) {
            b = mapRange(x, -1, 0, 255, 0, true);
            ele.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
            //ele.style.borderColor     = 'rgb(32, 32, 32)';
        }
        else {
            r = mapRange(x, 0, 1, 0, 255, true);
            ele.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
            //ele.style.borderColor     = 'rgb(223, 223, 223)';
        }
        ele.style.borderColor = 'rgb(32, 32, 32)';
    }
}

function setupGamepadEvents() {
    window.addEventListener('gamepadconnected', (event) => {
        console.log(`Gamepad connected at index ${event.gamepad.index}: ${event.gamepad.id}.`);
        updateGamepadState();
    });
    
    window.addEventListener('gamepaddisconnected', (event) => {
        console.log(`Gamepad disconnected from index ${event.gamepad.index}: ${event.gamepad.id}`);
        if (activeGamepadIndex === event.gamepad.index) {
            activeGamepadIndex = null;
            MyGamepad = null;
            console.log('Active gamepad has been disconnected.');
        }
        updateGamepadState();
    });
}

function createDebugGrid() {
    const container = document.getElementById('debug_area');
    container.style.display = 'grid';
    container.style.gridTemplateColumns = ` 1fr 2fr 1fr 2fr`;
    container.style.width = '100%';
    var ch = 0; var vr = 0;

    for (let row = 0; ch < channel.length || vr < variable.length; row++) {
        for (let col = 0; col < 4; col++) {
            const cell = document.createElement('div');
            cell.classList.add('dbggrid-cell');
            if (col == 0 || col == 2) {
                cell.classList.add('right-aligned');
                if (ch < 16) {
                    cell.innerHTML = "Ch " + (ch + 0).toString() + ":";
                    cell.id = `dbgcell_ch_lbl_${ch}`;
                }
                else {
                    cell.innerHTML = "Var " + (vr).toString() + ":";
                    cell.id = `dbgcell_var_lbl_${vr}`;
                }
            }
            else if (col == 1 || col == 3) {
                cell.innerHTML = "&nbsp;";
                if (ch < channel.length) {
                    cell.id = `dbgcell_ch_val_${ch}`;
                    ch += 1;
                }
                else {
                    cell.id = `dbgcell_var_val_${vr}`;
                    vr += 1;
                }
            }
            container.appendChild(cell);
        }
    }
}

function fillDebugCells() {
    for (let i = 0; i < channel.length; i++) {
        var cell = document.getElementById(`dbgcell_ch_val_${i}`);
        cell.innerHTML = channel[i].toString();
    }
    for (let i = 0; i < variable.length; i++) {
        var cell = document.getElementById(`dbgcell_var_val_${i}`);
        cell.innerHTML = variable[i].toString();
    }
}

//@@include("libs.js")
