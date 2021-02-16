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

class API_TUBE_TO_MP3 {};
API_TUBE_TO_MP3.API_HOST = 'http://35.202.184.164';
API_TUBE_TO_MP3.API_DOWNLOAD = '/api/dl';
API_TUBE_TO_MP3.API_STREAM = '/api/stream';

API_TUBE_TO_MP3.getVideoId = (url) => {
  // v=pDgflOcHNnM
  if (url == null) {
      return null;
  }        
  const m = url.match('v=(.*)');       
  console.log(m);
  if (m == null) {
      return null;
  }
  return m[1];
}

API_TUBE_TO_MP3.convert = (videoId) => {
  return new Promise((resolve,reject) => {
      fetch(API_TUBE_TO_MP3.API_HOST + API_TUBE_TO_MP3.API_DOWNLOAD + '/' +videoId)
          .then(response => {
              return response.json();
          })
          .then(data => {  resolve(data) })
          .catch(e => reject(e));
  });
}

API_TUBE_TO_MP3.stream = (videoId) => {
  return API_TUBE_TO_MP3.API_HOST + API_TUBE_TO_MP3.API_STREAM + '/' +videoId
}

Vue.use(window['vue-qriously']);

var app = new Vue({
    el: "#app",
    components: {
      'vueSlider': window[ 'vue-slider-component' ],
      'vue-channel-color-picker': window['vue-channel-color-picker']
    },
    data: {
      title: "ConnectedDoll",
      url: "http://connecteddoll.local",
      isLedOn: false,
      youtube: {
        url: 'https://www.youtube.com/watch?v=1WTy2yqKI4w',
        stream: 'http://35.202.184.164/api/stream/1WTy2yqKI4w'
      },
      volume: {
        min: 0,
        max: 100,
        value: 10
      },
      color: {
        type: "rgb",
        channels: [0, 0, 0]
      },
      leds: [
        {
          type: "rgb",
          channels: [0, 0, 0]
        },
        {
          type: "rgb",
          channels: [0, 0, 0]
        },
        {
          type: "rgb",
          channels: [0, 0, 0]
        },
        {
          type: "rgb",
          channels: [0, 0, 0]
        }
      ]
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
    watch:  {
      'volume.value': function (newValue, oldValue) {
        // console.log('volume.value', this.volume.value);
        API.request('/api/volume', {'volume': this.volume.value/100})
          .then((json)=>{
            console.log(json);
          })
          .catch((e) => {
            console.error(e);
          });
      }
    },
    methods: {
      updateLedColor(color) {
        this.color = color;
        this.leds[0] = JSON.parse(JSON.stringify(color));
        this.leds[1] = JSON.parse(JSON.stringify(color));
        this.leds[2] = JSON.parse(JSON.stringify(color));
        this.leds[3] = JSON.parse(JSON.stringify(color));
        console.log('updateLedColor', this.leds, color);
      },
      colorPickerOen(status){
        console.log('colorPickerOen', status);
        if (status == false) {
          this.isLedOn = false;
          this.clickLedToggle();
        }
      },
      clickLedToggle() {        
        // curl -X POST -H "Content-Type: application/json" -d '{"leds": ["255,255,255","255,255,255","255,255,255","255,255,255"]}' http://192.168.86.48/api/led
        this.isLedOn = !this.isLedOn;
        const data = {"leds":["0,0,0","0,0,0","0,0,0","0,0,0"]};
        if (this.isLedOn) {
          data['leds'][0] = this.leds[0]['channels'][0] + "," + this.leds[0]['channels'][1] + "," + this.leds[0]['channels'][2];
          data['leds'][1] = this.leds[1]['channels'][0] + "," + this.leds[1]['channels'][1] + "," + this.leds[1]['channels'][2];
          data['leds'][2] = this.leds[2]['channels'][0] + "," + this.leds[2]['channels'][1] + "," + this.leds[2]['channels'][2];
          data['leds'][3] = this.leds[3]['channels'][0] + "," + this.leds[3]['channels'][1] + "," + this.leds[3]['channels'][2];
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
        const data = {'path': '/mp3/d3_IcaDhcDM.mp3'};
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
      },
      clickPlayStopMp3() {
        const data = {};
        API.request("/api/stop/mp3", data)
          .then((json)=>{
            console.log(json);
          })
          .catch((e) => {
            console.error(e);
          });
      },
      clickPlayStreamMp3() {
        // 変換サーバにアクセスして stream URL を取得する
        const videoId = API_TUBE_TO_MP3.getVideoId(this.youtube.url);
        API_TUBE_TO_MP3.convert(videoId)
          .then(result => {
            // ストリーム URL 生成
            this.youtube.stream = API_TUBE_TO_MP3.stream(videoId);

            // 再生
            const data = {'url': this.youtube.stream};
            API.request("/api/play/stream/mp3", data)
              .then((json)=>{
                console.log(json);
              })
              .catch((e) => {
                console.error(e);
              });    
          })
          .catch(e => {
            console.error('error', e);
            alert('エラー ' + e);
          });
      }  
    }
  });