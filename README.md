# LayerAlchemy
## C++ Multichannel Nuke plugins
![logo](./icons/layer_alchemy_200px.png)


### Nuke installation for compiled builds

```code 
# add this to your init.py and adapt the path to where the package is uncompressed
nuke.pluginAddPath('/path/to/plugin/LayerAlchemy/nuke')
```


### Build instruction examples :

```code
git clone https://github.com/sebjacob/LayerAlchemy
```

_using environment variables :_
```code
export NUKE_ROOT=/Applications/Nuke11.3v4
export LAYER_ALCHEMY_DIR=/path/to/git/cloned/LayerAlchemy
export LAYER_ALCHEMY_BUILD_DIR=/a/new/folder/to/build/in
export LAYER_ALCHEMY_INSTALL_DIR=/a/new/folder/to/install/to

cd $LAYER_ALCHEMY_BUILD_DIR
cmake $LAYER_ALCHEMY_DIR -DNUKE_ROOT=$NUKE_ROOT -DCMAKE_INSTALL_PREFIX=$LAYER_ALCHEMY_INSTALL_DIR
```
_or no environment variables :_ 

```code
mkdir /a/new/folder/to/build/in
cd /a/new/folder/to/build/in
cmake /path/to/git/cloned/LayerAlchemyDir -DNUKE_ROOT=/Applications/Nuke11.3v4 -DCMAKE_INSTALL_PREFIX=/Path/to/install/to
```
```code
make # compile the code
make install # copies the compiled files to the install directory
make documentation # build the documentation, needs mkdocs (pip install mkdocs)
make package # creates a compressed file containing the project
```
