# Building, installation and testing

!!! note "once compiled or downloaded, add this to your init.py"
        There is a nuke folder in the install, this folder is what you need to tell Nuke to use
        
        _nuke.pluginAddPath('/path/to/LayerAlchemy/nuke')_

# Build instruction examples

First you must run cmake to configure the compilation

!!! example "_using environment variables :_"
            export NUKE_ROOT=/Applications/Nuke11.3v4
            export LAYER_ALCHEMY_DIR=/path/to/git/cloned/LayerAlchemy
            export LAYER_ALCHEMY_BUILD_DIR=/a/new/folder/to/build/in
            export LAYER_ALCHEMY_INSTALL_DIR=/a/new/folder/to/install/to
            cd $LAYER_ALCHEMY_BUILD_DIR
            cmake $LAYER_ALCHEMY_DIR -DNUKE_ROOT=$NUKE_ROOT -DCMAKE_INSTALL_PREFIX=$LAYER_ALCHEMY_INSTALL_DIR
!!! example "_no environment variables :_"
            mkdir /a/new/folder/to/build/in
            cd /a/new/folder/to/build/in
            cmake /path/to/git/cloned/LayerAlchemyDir -DNUKE_ROOT=/Applications/Nuke11.3v4 -DCMAKE_INSTALL_PREFIX=/Path/to/install/to

Then you can actually start compiling, or create a package.

!!! example "building"
            make # compile the code
            make documentation # build the documentation
            make install # copies the compiled files to the install directory
            make package # creates a compressed file containing the install directory for distribution

# Config Tools

The following commandline tools can help fine tune config files
[LayerTester](tools.md#LayerTester)
[ConfigTester](tools.md#ConfigTester)