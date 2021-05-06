
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
