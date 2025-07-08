# Builds project in the manylinux build container and copies the output
# wheel to ./build/wheelhouse.
#
# To upload the package to pypi after that, run:
# $ twine upload build/wheelhouse/*

sudo docker build -t build_container .
mkdir -p build
cid=$(sudo docker create build_container:latest)
rm -rf ./build/wheelhouse
sudo docker cp "$cid":/workspace/dist/wheelhouse ./build
sudo chown -R $USER:$USER ./build/wheelhouse
sudo docker rm "$cid"

