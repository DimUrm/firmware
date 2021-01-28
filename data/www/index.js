class API {};

API.request = (url, data) => {
  return new Promise((resolut,reject) =>{
    const param  = {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify(data)
    };
    console.log('param', param);

    fetch(url, param)
      .then((res)=>{
        return( res.json() );
      })
      .then((json)=>{
        resolut(json);
      })
      .catch((e) => {
        reject(e);
      });
  });
};

Vue.use(window['vue-qriously']);
var app = new Vue({
    el: "#app",
    data: {
      title: "ConnectedDoll",
      url: "http://connecteddoll.local",
      isLedOn: false
    },
    created: function() {
      console.log('created');
      // IPAddress 取得
      API.request('/api/ip', {})
        .then((json)=>{
          console.log(json);
          this.url = 'http://' + json['ip'];
        })
        .catch((e) => {
          console.error(e);
        });
    },
    methods: {
        clickLedToggle() {
          // curl -X POST -H "Content-Type: application/json" -d '{"leds": ["255,255,255","255,255,255","255,255,255","255,255,255"]}' http://192.168.86.48/api/led
          this.isLedOn = !this.isLedOn;
          const data = {"leds":["0,0,0","0,0,0","0,0,0","0,0,0"]};
          if (this.isLedOn) {
            data['leds'][0] = "255,255,255";
            data['leds'][1] = "255,255,255";
            data['leds'][2] = "255,255,255";
            data['leds'][3] = "255,255,255";
          }
          API.request("/api/led", data)
            .then((json)=>{
              console.log(json);
            })
            .catch((e) => {
              console.error(e);
            });
        },
        clickPlayLocalMp3() {
          const data = {'path': '/d3_IcaDhcDM.mp3'};
          API.request("/api/play/mp3", data)
            .then((json)=>{
              console.log(json);
            })
            .catch((e) => {
              console.error(e);
            });
        },
        clickPlayUrlMp3() {
          const data = {'url': 'https://file-examples-com.github.io/uploads/2017/11/file_example_MP3_700KB.mp3'};
          API.request("/api/play/url/mp3", data)
            .then((json)=>{
              console.log(json);
            })
            .catch((e) => {
              console.error(e);
            });
        }        
    }
  });