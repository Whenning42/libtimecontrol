source .env

sudo docker build -t build_container .
mkdir -p build
cid=$(sudo docker create build_container:latest)
rm -rf ./build/wheelhouse
sudo docker cp "$cid":/workspace/dist/wheelhouse ./build
sudo chown -R $USER:$USER ./build/wheelhouse
sudo docker rm "$cid"

# twine upload --repository testpypi build/wheelhouse/*
