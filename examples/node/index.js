"use strict"

const send_host = process.argv[2];
const send_port = 54345;

console.log("send_host:port " + send_host + ":" + send_port);

const osc = require('node-osc');
const client_send = new osc.Client(send_host, send_port);
function send() {
    // Wi-Fi 設定の初期化をする場合
    //client_send.send('/status/reset');
    //console.log('/status/reset');

    client_send.send('/status/dir');
    console.log('/status/dir');
    
/*
    client_send.send('/status/volume', 0.3);
    console.log('/status/volume');

    client_send.send('/status/color', "255,0,0", "0,0,255", "0,255,0", "255,0,255");
    console.log('/status/color');

    setTimeout( function() {
        client_send.send('/status/color', "0,0,0", "0,0,0", "0,0,0", "0,0,0");
        console.log('/status/color');    
    },1000);
*/
    // client_send.send('/status/play/mp3', "/sound.mp3");
    // console.log('/status/play/mp3');
    
    const mp3url = "https://file-examples-com.github.io/uploads/2017/11/file_example_MP3_700KB.mp3";
    client_send.send('/status/play/url/mp3', mp3url);
    console.log('/status/play/url/mp3');
}

setInterval(send, 5000);
