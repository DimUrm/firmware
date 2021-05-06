Vue.use(window['vue-qriously']);

var app = new Vue({
    el: "#app",
    components: {
      'vueSlider': window[ 'vue-slider-component' ],
      'vue-channel-color-picker': window['vue-channel-color-picker']
    },
    data: {
      config: Config,
      title: "ConnectedDoll",
      isActiveTab: "Aram",
      qrCodeUrl: "http://connecteddoll.local",
      aram: {
        title: "編集",
        selectItem: {
          index: 0,
          timeHHMM: "",
          title: "",
          playbackTime: "",
          color: "0,0,0",
          effect: "",
          url: ""
        },
        defaultItem: {
          index: 0,
          timeHHMM: "",
          title: "",
          playbackTime: "",
          color: "0,0,0",
          effect: "",
          url: ""
        },
        listTimeHHMM: [
          "---",
          "00:00","00:30",
          "01:00","01:30",
          "02:00","02:30",
          "03:00","03:30",
          "04:00","04:30",
          "05:00","05:30",
          "06:00","06:30",
          "07:00","07:30",
          "08:00","08:30",
          "09:00","09:30",
          "10:00","10:30",
          "11:00","11:30",
          "12:00","12:30",

          "13:00","13:30",
          "14:00","14:30",
          "15:00","15:30",
          "16:00","16:30",
          "17:00","17:30",
          "18:00","18:30",
          "19:00","19:30",
          "20:00","20:30",
          "21:00","21:30",
          "22:00","22:30",
          "23:00","23:30",
        ],
        listColor: [
          "255,0,0",
          "0,255,0",
          "0,0,255",
        ],
        listEffect: [
          "effect_repetition.png",
          "effect_repetition.png",
          "effect_repetition.png",
        ],
        items: [
          {
            "index": 0,
            "timeHHMM": "08:00",
            "title": "おきてぇ",
            "playbackTime": "0:39",
            "color": "255,255,255",
            "effect": "effect_repetition.png",
            "url": "https://www.youtube.com/watch?v=DvkRoXRbqFw"
          }
        ]
      }
    },
    created: function() {
      console.log('created', this.config);
      // アラーム設定をロードする
      this.loadAramJson();
      // QRコード URL取得
      this.setQRCodeURL();
    },
    watch:  {

    },
    methods: {
      loadAramJson(){
        API.request("/api/aram", [])
          .then((json)=>{
            console.log("aram", json);
            this.aram.items = [].concat(json);
          })
          .catch((e) => {
            console.error(e);
          });
      },
      clickTab(name) {
        this.isActiveTab = name;
      },
      setQRCodeURL() {
        // IPAddress 取得 して QRコード用のURLを設定する
        API.request('/api/ip', {})
        .then((json)=>{
          console.log(json);
          this.qrCodeUrl = 'http://' + json['ip'];
        })
        .catch((e) => {
          console.error(e);
        });
      },
      clickModalToggle(id) {
        const modal = document.getElementById(id);
        modal.classList.toggle('is-active');
      },
      clickAramEdit(item) {
        this.aram.title = "編集";

        this.aram.selectItem = Object.assign({},item);

        this.clickModalToggle("modal-aram-edit");
      },
      clickAramSave() {
        console.log('clickAramSave', this.aram.selectItem);
        this.saveAramItem(this.aram.selectItem)
          .then((json) => {
            console.log('saveAramItem result', json);
            this.clickModalToggle("modal-aram-edit");
          })
          .catch((e) => {
            console.error('saveAramItem e', e);
            alert('エラーが発生しました ' + e);
          });
      },
      clickAramDelete(item){
        this.aram.title = "削除";

        const index = this.aram.selectItem.index;
        const result = confirm(`${item.title} を 削除しますがよろしいですか？`);
        if (result) {
          // アラーム設定 リセット
          this.aram.selectItem = Object.assign({},this.aram.defaultItem);
          this.aram.selectItem.index = index;

          // 保存処理
          this.saveAramItem(this.aram.selectItem)
            .then((json) => {
              console.log('saveAramItem json', json);
              this.aram.items = [].concat(json);
            })
            .catch((e) => {
              console.error('saveAramItem e', e);
              alert('エラーが発生しました ' + e);
            });
        }
      },
      clickColorPicker() {
        // TOTO カラーピッカー, プリセットの色とエフェクト選択
      },
      saveAramItem(item) {
        // 更新する
        this.aram.items[item.index] = Object.assign({},item);
        console.log("aram.items",  this.aram.items);

        return API.request("/api/aram", this.aram.items);
      },
      clickPlayStreamMp3() {
        // 視聴
        if (this.aram.selectItem.url == ""){
          return;
        }

        // 変換サーバにアクセスして stream URL を取得する
        const videoId = API_TUBE_TO_MP3.getVideoId(this.aram.selectItem.url);
        API_TUBE_TO_MP3.convert(videoId)
          .then(result => {
            // ストリーム URL 生成
            const streamURL = API_TUBE_TO_MP3.stream(videoId);

            // 再生
            const data = {'url': streamURL};
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
      },
      colorIcon(c) {
        return {"background-color": "rgb(" +c+ ")"};
      },
      effectIcon(e) {
        return {"background-image": "url(" +e+ ")"};
      },
      selectColor(c){
        this.aram.selectItem.color = c;

        console.log("selectItem ", this.aram.selectItem);
      },
      selectEffect(e){
        this.aram.selectItem.effect = e;

        console.log("selectItem ", this.aram.selectItem);
      }      
    }
  });