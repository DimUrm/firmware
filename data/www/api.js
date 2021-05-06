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
    console.log('protocol', window.location.protocol);
    console.log('url', url);
    console.log('param', param);
    if (window.location.protocol == "file:") {
      // ローカルでのテストの場合 リジェクトとする
      reject( Error('ローカルでの実行') );
      return;
    }
    try {
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
    }
    catch(e) {
      reject(e);
    }

  });
};