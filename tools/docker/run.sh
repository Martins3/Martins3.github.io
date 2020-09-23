 export http_proxy="http://127.0.0.1:8888";
 export HTTP_PROXY="http://127.0.0.1:8888";
export https_proxy="https://127.0.0.1:8888";
export HTTPS_PROXY="https://127.0.0.1:8888"

docker build --network host -f Dockerfile . 
