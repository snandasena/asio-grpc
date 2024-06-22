# Basic configs and environment setup

### Configure Conan and install dependencies

```bash

# Create python environment
python -m virtualenv dev
source dev/bin/activate

python -m pip install conan

# Configure Conan 
conan profile detect # default profile
# If you want you can edit the default profile
# Default profile location - ~/.conan2/profiles/default
# Followings are my dev configs
#[settings]
#arch=x86_64
#build_type=Release
#compiler=gcc
#compiler.cppstd=20
#compiler.libcxx=libstdc++11
#compiler.version=13
#os=Linux

# Clone the project and  Install dependencies
git clone https://github.com/snandasena/asio-grpc.git
cd asio-grpc

conan install . --build=missing # Will take couple of minutes

```

### Build and run examples

```bash

cd build
cmake ..  -DCMAKE_BUILD_TYPE=Release

cmake --build .

# Find the generated binaries from relevant build folders

```
