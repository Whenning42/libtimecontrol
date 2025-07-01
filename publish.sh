source .env

sudo docker build -t build_container .
mkdir -p build
cid=$(docker create build_container:latest)
docker cp "$cid":/workspace/dist/wheelhouse ./build
docker rm "$cid"

# twine upload --repository testpypi build/wheelhouse/*
