# clumsy

__clumsy makes your network condition on Windows significantly worse, but in a managed and interactive manner.__

Leveraging the awesome [WinDivert](http://reqrypt.org/windivert.html), clumsy stops living network packets and capture them, lag/drop/tamper/.. the packets on demand, then send them away. Whether you want to track down weird bugs related to broken network, or evaluate your application on poor connections, clumsy will come in handy:

* No installation.
* No need for proxy setup or code change in your application.
* System wide network capturing means it works on any application.
* Works even if you're offline (ie, connecting from localhost to localhost).
* Your application keeps running, while clumsy can start and stop anytime.
* Interactive control how bad the network can be, with enough visual feedback to tell you what's going on.

See [this page](http://jagt.github.io/clumsy) for more info and build instructions.

## License

MIT

## InTouchHealth Fork Notes

### Usage Notes:
* You can now open multiple Clumsy instances.
    * This was possible in earlier versions, then was lost, but now it's back.
    * Note that we haven't carefully tested the behavior of overlapping filters, so use with care.
* Bandwidth cap is now available.
    * The cap is shared across up and down traffic, so it's recommended to use a second Clumsy instance for fully independent up vs. down control.
    * This feature is still somewhat experimental, and may not be 100% stable.

### Build instructions for Windows 10 with VS 2019:
* Clone the repo.
* Download the most recent 5.x.x Windows version of Premake from here: https://premake.github.io/download.html, and extract the EXE into your local clumsy repo folder.
* From a command prompt in the clumsy repo folder, execute 'premake5 vs2010'. If you see any Windows Defender or AV warnings, bypass them to execute the command.
* Open the 'clumsy/build/clumsy.sln' file in VS 2019, and accept the default recommendations for updating the Windows SDK Version and Platform Toolset.
* You should now be able to build and debug the various configurations from VS 2019.
* You may delete the premake5.exe file at this point if you'd like.

### Fork History:
* Forked from "offical" jagt/clumsy into codyherzog/clumsy.
* Pulled in changes from crunkyball/clumsy to get WinDivert library upgrade which is required for some of our advanced filters.
* Pulled in changes from rorlork/clumsy to get improvements to bandwidth cap feature.
* Modified the bandwidth cap code, because it had some bugs and wasn't optimal for ITH usage.
* Made a few other tweaks, such as allowing multiple instances.
* Forked from codyherzog/clumsy to IntouchHealth/clumsy, where ongoing ITH development can take place.
* The "official" jagt/clumsy project seems to be dead. It hasn't been touched in many years. If it ever picks up again, we should try to get our changes into it.
