function am32_init()
{
    document.getElementById("btn_readbinfile").addEventListener("change", readBinFile, false);
    document.getElementById("tbl_checkboxes").innerHTML      = make_all_checkboxes(plain_checkboxes);
    document.getElementById("tbl_sliders").innerHTML         = make_all_sliders(plain_sliders);
    document.getElementById("tbl_extracheckboxes").innerHTML = make_all_checkboxes(extra_checkboxes);
    document.getElementById("tbl_extrasliders").innerHTML    = make_all_sliders(extra_sliders);

    document.getElementById("chk_drivebyrpm").addEventListener('change', function() {
        enabledisable_elements_with("_speedctrl", this.checked);
    });
    document.getElementById("chk_variablepwm").addEventListener('change', function() {
        enabledisable_elements_with("_pwmfrequency", !this.checked);
    });
    document.getElementById("chk_brushedmode").addEventListener('change', function() {
        enabledisable_elements_with("chk_dualbrushedmode", this.checked);
        if (this.checked == false) {
            document.getElementById("chk_dualbrushedmode").checked = false;
            document.getElementById("chk_dualbrushedmode").dispatchEvent(new Event('change'));
        }
    });
    document.getElementById("chk_lowvoltagecutoff").addEventListener('change', function() {
        enabledisable_elements_with("txt_lowvoltagecutoff", this.checked);
        enabledisable_elements_with("sld_lowvoltagecutoff", this.checked);
    });
    document.getElementById("chk_dualbrushedmode").addEventListener('change', function() {
        if (this.checked) {
            document.getElementById("chk_brushedmode").checked = true;
            let inputmode = document.getElementById("drop_rcinput").value;
            if (inputmode != "x_3") {
                document.getElementById("drop_rcinput").value = "x_3";
            }
            drop_rcinput_onchange();
        }
        else {
            drop_rcinput_onchange();
        }
    });

    fn_saveByteArray = (function () {
        var a = document.createElement("a");
        document.body.appendChild(a);
        a.style = "display: none";
        return function (data, name) {
            var blob = new Blob([data], {type: "octet/stream"});
            var url = window.URL.createObjectURL(blob);
            a.href = url;
            a.download = name;
            a.click();
            window.URL.revokeObjectURL(url);
            console.log("Local binary file saved");
        };
    }());

    am32_init2();
}

async function am32_init2()
{
    let data;
    if (!isRunningLocally()) {
        let formData = new FormData();
        formData.append("action", 0);
        let response = await fetch("/am32io", { method: 'POST', body: {formData}});
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        data = await response.text();
    }
    else {
        data = "PWM 1 2,PWM 3 4,SERRX 5, SERTX 6";
    }
    let pin_strs = data.split(',');
    let select = document.getElementById('drop_selpin');
    for (let pin_str of pin_strs) {
        try {
            let opt = document.createElement('option');
            let pin_parts = pin_str.split(' ');
            if (pin_parts[0] == "PWM") {
                let pin_num = parseInt(pin_parts[2]);
                let ch_num = parseInt(pin_parts[1]);
                opt.value = pin_num;
                opt.innerHTML = `PWM CH-${ch_num} PIN-${pin_num}`;
            }
            else if (pin_parts[0].startsWith("SERTX")) {
                let pin_num = parseInt(pin_parts[1]);
                opt.value = pin_num;
                opt.innerHTML = `SERIAL-TX PIN-${pin_num}`;
            }
            else if (pin_parts[0].startsWith("SERRX")) {
                let pin_num = parseInt(pin_parts[1]);
                opt.value = pin_num;
                opt.innerHTML = `SERIAL-RX PIN-${pin_num}`;
            }
            select.appendChild(opt);
        }
        catch (e) {
        }
    }
    document.getElementById("div_loading").style.display = "none";
    document.getElementById("div_maincontent").style.display = "block";
    document.getElementById("div_escconnect").style.display = "block";
    if (!isRunningLocally())
    {
        //document.getElementById("btn_serwrite").style.display = "none";
        document.getElementById("btn_serwrite").disabled = true;
        document.getElementById("div_maincontent").style.display = "none";
        document.getElementById("fld_crsfchannels").style.display = "none";
        document.getElementById("fld_crsf2channels").style.display = "none";
        document.getElementById("div_experimentalextras").style.display = "none";
    }
    document.getElementById("chk_drivebyrpm").dispatchEvent(new Event('change'));
    document.getElementById("chk_variablepwm").dispatchEvent(new Event('change'));
    document.getElementById("chk_lowvoltagecutoff").dispatchEvent(new Event('change'));
    document.getElementById("chk_brushedmode").dispatchEvent(new Event('change'));
    document.getElementById("chk_dualbrushedmode").dispatchEvent(new Event('change'));
    drop_rcinput_onchange();
}

let mcu_data = [
    {
        "name": "Generic 32K",
        "signature": [0x1F, 0x06],
        "eeprom_start": 0x7C00,
        "addr_multi": 1
    },
    {
        "name": "Generic 64K",
        "signature": [0x35, 0x06],
        "eeprom_start": 0xF800,
        "addr_multi": 1
    },
    {
        "name": "Generic 128K",
        "signatures": [0x2B,0x06],
        "eeprom_start": 0xF800,
        "addr_multi": 4
    }
];

let current_chip = null;

let flash_write_chunk = 128;
let eeprom_useful_length = 0x30;
let eeprom_total_length = 0xB0; // 176

let plain_checkboxes = [
    // Name                  , def  , byte
    ["Reverse Rotation",       false, 17, ],
    ["Complementary PWM",      true , 20, ],
    ["Variable PWM",           true , 21, ],
    ["Bi-Directional",         false, 18, ],
    ["Stuck Rotor Protection", false, 22, ],
    ["Brake On Stop",          false, 28, ],
    ["Stall Protection",       false, 29, ],
    ["Sinusoidal Startup",     false, 19, ],
    ["Telemetry 30ms",         false, 31, ],
    ["Auto Advance",           false, 39, ],
    ["Low Voltage Cutoff",     false, 36, ],
    ["Double Tap Reverse",     false, 38, ],
];

let extra_checkboxes = [
    ["Automatic Timing",       false, 53, ],
    ["Drive by RPM",           false, 54, ],
    ["Brushed Mode",           false, 64, ],
    ["Dual Brushed Mode",      false, 69, ],
];

let plain_sliders = [
    // Name                  ,  def ,  min ,  max ,   step, offset, byte, readonly
    ["Timing Advance"        ,     2,     0,     3,      1,      0,   23, false, ],
    ["Motor KV"              ,  2000,    20, 10220,     40,     20,   26, false, ],
    ["Motor Poles"           ,    14,     0,   255,      1,      0,   27, false, ],
    ["Startup Power"         ,   100,    50,   150,      1,      0,   25, false, ],
    ["PWM Frequency"         ,    24,     8,    48,      1,      0,   24, false, ],
    ["Beep Volume"           ,     7,     0,    10,      1,      0,   30, false, ],
    ["Stopped Brake Level"   ,    10,     0,    10,      1,      0,   41, false, ],
    ["Running Brake Level"   ,     9,     0,     9,      1,      0,   42, false, ],
    ["Sine Startup Range"    ,     5,     5,    25,      1,      0,   40, false, ],
    ["Sine Mode Power"       ,     5,     0,    10,      1,      0,   45, false, ],
    ["Servo Low Thresh"      ,  1000,   750,  1500,      2,    750,   32, false, ],
    ["Servo High Thresh"     ,  2000,  1750,  2260,      2,   1750,   33, false, ],
    ["Servo Neutral"         ,  1500,  1374,  1629,      1,   1374,   34, false, ],
    ["Servo Dead Band"       ,     0,     0,   255,      1,      0,   35, false, ],
    ["Low Voltage Cutoff"    ,   330,   250,   505,      1,    250,   37, false, ],
    ["Temperature Limit C"   ,   255,     0,   255,      1,      0,   43, false, ],
    ["Current Limit Amps"    ,     0,     0,   202,      2,      0,   44, false, ],
];

let extra_sliders = [
//    ["Speed Ctrl Min Input"  ,    47,    47,  5147,     20,     47,   55, false, ],
//    ["Speed Ctrl Max Input"  ,  5147,    47,  5147,     20,     47,   56, false, ],
    ["Speed Ctrl Min RPM"    ,     0,     0, 25500,    100,      0,   57, false, ],
    ["Speed Ctrl Max RPM"    , 51000,     0, 51000,    200,      0,   58, false, ],
    ["Speed Ctrl PID Kp"     ,    10,     0,   255,      1,      0,   59, false, ],
    ["Speed Ctrl PID Ki"     ,     0,     0,   255,      1,      0,   60, false, ],
    ["Speed Ctrl PID Kd"     ,     0,     0,   255,      1,      0,   61, false, ],
    ["ROTC Divider"          ,     0,     0,   255,      1,      0,   62, false, ],
    ["Minimum Duty Cycle"    ,     0,     0,  5100,     20,      0,   63, false, ],
    ["RGB LED Red"           ,     0,     0,   255,      1,      0,   66, false, ],
    ["RGB LED Green"         ,   255,     0,   255,      1,      0,   67, false, ],
    ["RGB LED Blue"          ,     0,     0,   255,      1,      0,   68, false, ],
];

let all_sliders = [];
all_sliders = all_sliders.concat(plain_sliders, extra_sliders);
let all_checkboxes = [];
all_checkboxes = all_checkboxes.concat(plain_checkboxes, extra_checkboxes);

let fn_saveByteArray; // this will be a function

let ui_locked = false;

function isRunningLocally() {
    return window.location.hostname === "localhost" || window.location.hostname === "127.0.0.1" || window.location.protocol === "file:";
}

function text_to_id(t)
{
    return t.toLowerCase().replaceAll(" ", "").replaceAll("-", "").replaceAll(".", "");
}

function make_checkbox(x)
{
    let t;
    t = "<div style=\"display: table-cell; text-align: right;\"><label for=\"chk_" + text_to_id(x[0]) + "\">" + x[0] + "</label></div>\r\n";
    t += "<div style=\"display: table-cell; text-align: left\"><input id=\"chk_" + text_to_id(x[0]) + "\" type=\"checkbox\"";
    if (x[1])
    {
        t += " checked=\"checked\"";
    }
    t += " /></div>\r\n";
    return t;
}

function make_slider(x, i)
{
    let t;
    t = "<div style=\"display: table-cell; text-align: right; padding-right:5pt;\"><label for=\"txt_" + text_to_id(x[0]) + "\">" + x[0] + "</label></div>\r\n";
    t += "<div style=\"display: table-cell; text-align: left; padding-right: 10pt;\"><input id=\"txt_" + text_to_id(x[0]) + "\" type=\"number\" style=\"width:100%; text-align: right;\"";
    t += "min=\"" + x[2] + "\" max=\"" + x[3] + "\" value=\"" + x[1] + "\" step=\"" + x[4] + "\" ";
    t += "onchange=\"txt_onchange('" + text_to_id(x[0]) + "')\"";
    if (x[7]) {
        t += " readonly disabled";
    }
    t += " /></div>\r\n";
    t += "<div style=\"display: table-cell;\"><input id=\"sld_" + text_to_id(x[0]) + "\" type=\"range\" ";
    t += "min=\"" + x[2] + "\" max=\"" + x[3] + "\" value=\"" + x[1] + "\" step=\"" + x[4] + "\" ";
    t += "onchange=\"sld_onchange('" + text_to_id(x[0]) + "')\"";
    t += " oninput=\"sld_onchange('" + text_to_id(x[0]) + "')\"";
    if (x[7]) {
        t += " readonly disabled";
    }
    t += " /></div>\r\n";
    t += "<div style=\"display: table-cell; padding-left:5pt;\">def = " + x[1] + "</div>\r\n";
    return t;
}

function make_all_checkboxes(chk_arr)
{
    let t = "";
    for (let i = 0; i < chk_arr.length; i++)
    {
        t += "<div style=\"display: table-row;\">\r\n";
        t += make_checkbox(chk_arr[i]);
        t += "</div>\r\n";
    }
    return t;
}

function make_all_sliders(sld_arr)
{
    let t = "";
    for (let i = 0; i < sld_arr.length; i++)
    {
        t += "<div style=\"display: table-row;\">\r\n";
        t += make_slider(sld_arr[i], i);
        t += "</div>\r\n";
    }
    return t;
}

function txt_onchange(i)
{
    if (ui_locked) {
        return;
    }
    ui_locked = true;
    for (let sld of all_sliders)
    {
        if (i == text_to_id(sld[0]))
        {
            let v = document.getElementById("txt_" + text_to_id(sld[0])).value;
            if (v < sld[2])
            {
                v = sld[2];
            }
            if (v > sld[3])
            {
                v = sld[3];
            }
            document.getElementById("sld_" + text_to_id(sld[0])).value = v;
        }
    }
    ui_locked = false;
}

function sld_onchange(i)
{
    if (ui_locked) {
        return;
    }
    ui_locked = true;
    for (let sld of all_sliders)
    {
        if (i == text_to_id(sld[0]))
        {
            let v = document.getElementById("sld_" + text_to_id(sld[0])).value;
            if (v < sld[2])
            {
                v = sld[2];
            }
            if (v > sld[3])
            {
                v = sld[3];
            }
            document.getElementById("txt_" + text_to_id(sld[0])).value = v;
        }
    }
    ui_locked = false;
}

function drop_rcinput_onchange()
{
    let v = document.getElementById("drop_rcinput").value;
    let can = isRunningLocally();
    if (can == false)
    {
        if (current_chip != null)
        {
            if (current_chip["fw_version_major"] >= 3 && current_chip["eeprom_layout"] >= 5) {
                can = true;
            }
        }
    }

    if (can && v == "x_3") {
        document.getElementById("fld_crsfchannels").style.display = "block";
        if (document.getElementById("chk_dualbrushedmode").checked) {
            document.getElementById("fld_crsf2channels").style.display = "block";
        }
        else {
            document.getElementById("fld_crsf2channels").style.display = "none";
        }
    }
    else {
        document.getElementById("fld_crsfchannels").style.display = "none";
        document.getElementById("fld_crsf2channels").style.display = "none";
    }
}

function enabledisable_elements_with(name, sts)
{
    let inputs = document.getElementsByTagName('input');
    for (var i = 0; i < inputs.length; i++) {
        // Check if the ID contains the search string
        if (inputs[i].id.includes(name)) {
            // Disable the input element
            inputs[i].disabled = !sts;
        }
    }
}

function readBinFile(e)
{
    let file = e.target.files[0];
    if (!file) {
        return;
    }
    let reader = new FileReader();
    reader.onload = function(e) {
        let barr = new Uint8Array(e.target.result);
        readBin(barr, true);
        console.log("Local binary file loaded");
        document.getElementById("btn_readbinfile").value = "";
    };
    reader.readAsArrayBuffer(file);
}

function readBin(barr, isFile)
{
    let dbg_txt = "";

    try {
    if (barr[0] == 0 || barr[0] == 0xFF) {
        dbg_txt += "warning: EEPROM appears empty\r\n";
    }
    if (barr[1] == 0 || barr[1] == 0xFF) {
        dbg_txt += "warning: EEPROM has no layout indicator\r\n";
    }
    if (barr[2] == 0 || barr[2] == 0xFF) {
        dbg_txt += "warning: bootloader version invalid\r\n";
    }

    if (isFile == false)
    {
        if (current_chip == null) {
            current_chip = {};
        }
        current_chip["eeprom_layout"] = barr[1];
        current_chip["bootloader_version"] = barr[2];
        current_chip["fw_version_major"] = barr[3];
        current_chip["fw_version_minor"] = barr[4];
    }

    for (let i = 0; i < all_checkboxes.length; i++)
    {
        let c = all_checkboxes[i];
        let ele = document.getElementById("chk_" + text_to_id(c[0]));
        ele.checked = barr[c[2]] != 0;
    }

    for (let i = 0; i < all_sliders.length; i++)
    {
        let sld = all_sliders[i];
        let bidx = sld[6];
        let eles = document.getElementById("sld_" + text_to_id(sld[0]));
        let elet = document.getElementById("txt_" + text_to_id(sld[0]));
        let val = barr[bidx];
        val *= sld[4];
        val += sld[5];
        if (val < sld[2] || val > sld[3])
        {
            dbg_txt += "\"" + sld[0] + "\" value " + val + " is out of range\r\n";
            if (val < sld[2])
            {
                val = sld[2];
            }
            if (val > sld[3])
            {
                val = sld[3];
            }
        }
        elet.value = val;
        txt_onchange(i);
    }

    let drop_rcinput = document.getElementById("drop_rcinput");
    drop_rcinput.value = "x_" + barr[46].toString();

    let txt_devicename = document.getElementById("txt_devicename");
    let dev_name = "";
    for (let i = 0; i < 12; i++)
    {
        let x = barr[5 + i];
        if (x == 0 || x == 0xFF) {
            break;
        }
        dev_name += String.fromCharCode(x);
    }
    txt_devicename.value = dev_name;

    let txt_crsfchannel = document.getElementById("txt_crsfchannel");
    txt_crsfchannel.value = barr[65] + 1;
    let txt_crsf2channel = document.getElementById("txt_crsf2channel");
    txt_crsf2channel.value = barr[70] + 1;

    }
    catch (e) {
        dbg_txt += "ERROR while reading binary: " + e.toString();
    }

    if (dbg_txt.length > 0) {
        console.log(dbg_txt);
    }
}

function generateBin()
{
    let dbg_txt = "";
    let buffer = new ArrayBuffer(eeprom_total_length);
    let buffer8 = new Uint8Array(buffer);
    for (let i = 0; i < buffer8.length; i++)
    {
        buffer8[i] = 0xFF;
    }
    buffer8[0] = 1; // indicate filled

    if (current_chip != null) {
        buffer8[1] = current_chip["eeprom_layout"];
        buffer8[2] = current_chip["bootloader_version"];
        buffer8[3] = current_chip["fw_version_major"];
        buffer8[4] = current_chip["fw_version_minor"];
    }

    for (let i = 0; i < all_checkboxes.length; i++)
    {
        let c = all_checkboxes[i];
        let ele = document.getElementById("chk_" + text_to_id(c[0]));
        buffer8[c[2]] = ele.checked ? 1 : 0;
        if (c[2] == 64 && ele.checked) {
            buffer8[c[2]] = 85;
        }
        if (c[2] == 69 && ele.checked) {
            buffer8[c[2]] = 85;
        }
    }
    for (let i = 0; i < all_sliders.length; i++)
    {
        let sld = all_sliders[i];
        let ele = document.getElementById("txt_" + text_to_id(sld[0]));
        let val = ele.value;
        val -= sld[5];
        val /= sld[4];
        if (val < 0 || val > 255) {
            dbg_txt += "\"" + sld[0] + "\" byte value " + val + " is overflowing\r\n";
            if (val < 0) {
                val = 0;
            }
            if (val > 255) {
                val = 255;
            }
        }
        buffer8[sld[6]] = Math.round(val);
    }

    let drop_rcinput = document.getElementById("drop_rcinput");
    buffer8[46] = Math.round(parseInt(drop_rcinput.value.substring(2)));

    let txt_devicename = document.getElementById("txt_devicename");
    let i;
    for (i = 0; i < 12 && i < txt_devicename.value.length; i++)
    {
        buffer8[5 + i] = Math.round(txt_devicename.value.charCodeAt(i)) & 0xFF;
    }
    for (; i < 12; i++)
    {
        buffer8[5 + i] = 0;
    }

    let txt_crsfchannel = document.getElementById("txt_crsfchannel");
    buffer8[65] = txt_crsfchannel.value - 1;
    let txt_crsf2channel = document.getElementById("txt_crsf2channel");
    buffer8[70] = txt_crsf2channel.value - 1;

    if (dbg_txt.length > 0) {
        console.log(dbg_txt);
    }

    return buffer8;
}

function getBinFileName(fname, default_name)
{
    if (fname.length > 0) {
        if (filename_isValid(fname)) {
            if (fname.toLowerCase().endsWith(".bin") == false) {
                fname += ".bin";
            }
            while (fname.includes("..")) {
                fname = fname.replaceAll("..", ".");
            }
        }
        else {
            console.log("warning: invalid save file name, using default name instead");
            fname = "";
        }
    }
    if (fname.length <= 0) {
        fname = default_name;
    }
    return fname;
}

function saveBinFile()
{
    let fname = document.getElementById("txt_savefname").value;
    fn_saveByteArray(generateBin(), getBinFileName(fname, "am32-eeprom.bin"));
}

let filename_isValid=(function(){
  let rg1=/^[^\\/:\*\?"<>\|]+$/; // forbidden characters \ / : * ? " < > |
  let rg2=/^\./; // cannot start with dot (.)
  let rg3=/^(nul|prn|con|lpt[0-9]|com[0-9])(\.|$)/i; // forbidden file names
  return function filename_isValid(fname){
    return rg1.test(fname)&&!rg2.test(fname)&&!rg3.test(fname);
  }
})();

function toHexString(x)
{
    let hex = x.toString(16);
    while ((hex.length % 2) != 0) {
        hex = "0" + hex;
    }
    return hex.toUpperCase();
}

function fromHexString(x)
{
    let byteList = [];
    for (let i = 0; i < hexString.length; i += 2) {
        let byte = parseInt(hexString.substr(i, 2), 16);
        byteList.push(byte);
    }
    return byteList;
}

function objectToFormData(obj) {
    let formData = new FormData();
    for (let key in obj) {
        formData.append(key, obj[key]);
    }
    return formData;
}

function uint8ArrayMerge(x, y)
{
    if (x == null)
    {
        x = new Uint8Array(y.length);
        for (let i = 0; i < y.length; i++) {
            x[i] = y[i];
        }
    }
    else
    {
        let mergedArray = new Uint8Array(x.length + y.length);
        mergedArray.set(x);
        for (let i = 0, j = x.length; i < y.length && j < mergedArray.length; i++, j++) {
            mergedArray[j] = y[i];
        }
        x = mergedArray; 
    }
    return x;
}

function serport_genSetAddressCmd(adr)
{
    let x = [0xFF, 0x00, 0x00, 0x00];
    x[2] = (adr & 0xFF00) >> 8;
    x[3] = (adr & 0x00FF) >> 0;
    let crc = serport_genCrc(x);
    x.push((crc & 0x00FF) >> 0);
    x.push((crc & 0xFF00) >> 8);
    return x;
}

function serport_genSetBufferCmd(x256, len)
{
    let x = [0xFE, 0x00, x256, len];
    let crc = serport_genCrc(x);
    x.push((crc & 0x00FF) >> 0);
    x.push((crc & 0xFF00) >> 8);
    return x;
}

function serport_genReadCmd(rdLen)
{
    let x = [0x03, rdLen];
    let crc = serport_genCrc(x);
    x.push((crc & 0x00FF) >> 0);
    x.push((crc & 0xFF00) >> 8);
    return x;
}

function serport_genInitQuery()
{
    let x = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        '\r'.charCodeAt(0),
        'B' .charCodeAt(0),
        'L' .charCodeAt(0),
        'H' .charCodeAt(0),
        'e' .charCodeAt(0),
        'l' .charCodeAt(0),
        'i' .charCodeAt(0)];
    // I don't think this packet uses the same CRC calculation
    //let crc = serport_genCrc(x);
    //x.push((crc & 0x00FF) >> 0);
    //x.push((crc & 0xFF00) >> 8);
    // CRC should be 0xF4, 0x7D
    x.push(0xF4);
    x.push(0x7D);
    return x;
}

function serport_genPayload(bin, start, len)
{
    //let x = new Uint8Array(len + 2);
    let x = [];
    for (let i = 0; i < len; i++)
    {
        //x[i] = bin[start + i];
        x.push(bin[start + i]);
    }
    let crc = serport_genCrc(x);
    x[len    ] = ((crc & 0x00FF) >> 0);
    x[len + 1] = ((crc & 0xFF00) >> 8);
    return x;
}

function serport_genFlashCmd()
{
    let x = [0x01, 0x01];
    let crc = serport_genCrc(x);
    x.push((crc & 0x00FF) >> 0);
    x.push((crc & 0xFF00) >> 8);
    return x;
}

function serport_genRunCmd()
{
    let x = [0x00, 0x00, 0x00, 0x00];
    let crc = serport_genCrc(x);
    x.push((crc & 0x00FF) >> 0);
    x.push((crc & 0xFF00) >> 8);
    return x;
}

function serport_genCrc(barr)
{
    let crc16 = new Uint16Array(1);
    let xb = new Uint8Array(1);
    crc16[0] = 0;
    for (let i = 0; i < barr.length; i++)
    {
        xb[0] = barr[i] & 0xFF;
        for (let j = 0; j < 8; j++)
        {
            if (((xb[0] & 0x01) ^ (crc16[0] & 0x0001)) != 0) {
                crc16[0] = (crc16[0] >> 1) & 0xFFFF;
                crc16[0] = (crc16[0] ^ 0xA001) & 0xFFFF;
            }
            else {
                crc16[0] = (crc16[0] >> 1) & 0xFFFF;
            }
            xb[0] = (xb[0] >> 1) & 0xFF;
        }
    }
    return crc16[0] & 0xFFFF;
}

function serport_verifyCrc(barr)
{
    let contents = barr.slice(0, -3);
    let calculated_crc = serport_genCrc(contents);
    let received_crc = barr.slice(-3);
    received_crc = (received_crc[0]) + (received_crc[1] << 8);
    return calculated_crc == received_crc;
}

async function serport_ajax(msg, action, pinnum, tx_data, delay, tx_data2) {
    let objdata = {"action": action};

    if (pinnum !== undefined) {
        objdata["pin"] = pinnum;
    }
    if (tx_data !== undefined) {
        let s = toHexString(tx_data);
        objdata["len1"] = s.length / 2;
        objdata["data1"] = s;
    }
    if (delay !== undefined) {
        objdata["delay"] = delay;
    }
    if (tx_data2 !== undefined) {
        let s = toHexString(tx_data2);
        objdata["len2"] = s.length / 2;
        objdata["data2"] = s;
    }

    let frmdata = objectToFormData(objdata);
    let response = await fetch("/am32io", { method: 'POST', body: {frmdata}});
    if (!response.ok) {
        throw new Error(`HTTP error while performing "${msg}"! status: ${response.status}`);
    }
    data = await response.text();
    console.log("ajax resp \"" + msg + "\" from backend: " + data);
    if (action == 4) {
        if (data == "xx") {
            data = [];
        }
        else {
            data = fromHexString(data);
        }
    }
    return data;
}

async function serport_ajax_read(num_bytes, total_time) {
    let buffer = [];
    for (let i = 0; i < total_time; i += 100) {
        let chunk = serport_ajax("read", 4);
        buffer.concat(chunk);
        if (buffer.length >= num_bytes) {
            break;
        }
        await new Promise(resolve => setTimeout(resolve, 100));
    }
    return buffer;
}

async function serport_ajax_readAck(msg) {
    let b = await serport_ajax_read(1, 1000);
    if (b.length == 1) {
        if (b[0] == 0x30) {
            return true;
        }
    }
    if (msg !== undefined) {
        throw new Error(`did not get ACK for "${msg}"`);
    }
    return false;
}

async function serport_ajax_flashRead(start_addr, read_len, chunk_size, addr_multi)
{
    let buffer = [];
    let i = 0;
    let adr = start_addr;
    while (buffer.length < read_len)
    {
        let data = await serport_ajax("send set address", 5, serport_genSetAddressCmd(adr / addr_multi));
        await serport_ajax_readAck("set address");
        data = await serport_ajax("send read cmd", 5, serport_genReadCmd(chunk_size));
        let reply_size = chunk_size + 3;
        data = await serport_ajax_read(reply_size, 1000);
        let timedout = false;
        if (data.length < reply_size)
        {
            timedout = true;
        }
        if (data.length >= 4) {
            if (data[data.length - 1] != 0x30) {
                throw new Error(`did not get ACK during read of address ${adr}`);
            }
            if (serport_verifyCrc(data) == false) {
                throw new Error(`CRC failed during read of address ${adr}`);
            }
            let actual_data = data.slice(0, -3);
            buffer.concat(actual_data);
        }
        if (timedout)
        {
            return buffer;
        }
        adr += chunk_size;
    }
    return buffer;
}

function btn_connect_onclick()
{
    btn_connect_onclick_a();
}

async function btn_connect_onclick_a()
{
    try
    {
    document.getElementById("drop_selpin").disabled = true;
    document.getElementById("btn_connect").disabled = true;
    document.getElementById("btn_serwrite").disabled = true;
    let data;
    let pinnum = document.getElementById("drop_selpin").value;
    if (!isRunningLocally()) {
        data = await serport_ajax("setting pin low", 1, pinnum);
        await new Promise(resolve => setTimeout(resolve, 2000));
        data = await serport_ajax("setting pin high", 2);
        await new Promise(resolve => setTimeout(resolve, 2000));
        data = await serport_ajax("init serial port", 3);
        await new Promise(resolve => setTimeout(resolve, 200));
        data = await serport_ajax("send query", 5, serport_genInitQuery());
        let signature_bytes = await serport_ajax_read(9, 1000);
        if (signature_bytes.length < 9) {
            throw new Error(`signature bytes are too short (or timed-out reading signature)`);
        }
        let mcu = null;
        for (let m of mcu_data)
        {
            let sig = m["signature"];
            if (sig[0] == signature_bytes[4] && sig[1] == signature_bytes[5]) {
                mcu = m;
                break;
            }
        }
        if (mcu == null) {
            throw new Error(`signature bytes do not have a match`);
        }
        data = await serport_ajax_flashRead(mcu["eeprom_start"], eeprom_total_length, flash_write_chunk, mcu["addr_multi"]);
        readBin(data, false);
        if (current_chip != null) {
            current_chip["mcu"] = mcu;
        }

        data = await serport_ajax_flashRead(0x10C0, 8, 8, mcu["addr_multi"]);
        if (data[0] != 1 && data[0] != 2) {
            data = await serport_ajax_flashRead(mcu["eeprom_start"] - 32, 8, 8, mcu["addr_multi"]);
            let is_all_ascii = true;
            for (let si = 0; si < 4; si++) {
                if (data[si] < 32 || data[si] > 126) {
                    is_all_ascii = false;
                    break;
                }
            }
            if (is_all_ascii) {
                current_chip["eeprom_layout"] = 5;
                current_chip["fw_version_major"] = 3;
                current_chip["fw_version_minor"] = 0;
            }
        }

        document.getElementById("btn_serwrite").style.display = "block";
        document.getElementById("btn_serwrite").disabled = false;
        document.getElementById("div_maincontent").style.display = "block";

        if (current_chip != null)
        {
            if (current_chip["fw_version_major"] >= 3 && current_chip["eeprom_layout"] >= 5) {
                document.getElementById("div_experimentalextras").style.display = "block";
            }
            else {
                document.getElementById("div_experimentalextras").style.display = "none";
            }
        }
    }
    else {
        console.log("pretending to send to pin " + pinnum);
        await new Promise(resolve => setTimeout(resolve, 2000));
        document.getElementById("btn_serwrite").style.display = "block";
        document.getElementById("btn_serwrite").disabled = false;
        document.getElementById("div_maincontent").style.display = "block";
    }
    }
    catch (e) {
        alert("ERROR: " + e.toString());
    }

    document.getElementById("drop_selpin").disabled = false;
    document.getElementById("btn_connect").disabled = false;
}