# NFS Most Wanted - XenonEffects port

![XenonEffects.png](Thumbnail\XenonEffects.png)

This is an attempt to port and implement XenonEffect particle emitters to NFS Most Wanted PC version.

## What are XenonEffects?

XenonEffects are an extension of the existing particle emitter system in Speed based games since Most Wanted. They primarily consist of extra sparks and contrails (wind effect) behind the car.

As the name implies, these effects were developed firstly for the Xbox 360, but found their way back onto the (then) current-gen console platforms.

This feature was famously not included in the PC version of NFS Most Wanted and this plugin attempts to bring them back by backporting the code from NFS Carbon into NFS Most Wanted.

## What works

- General rendering - implemented via DrawIndexedPrimitive

- Sparks work

- Contrails work

- Contrail status updating in CarRenderConn

- Particle effect list initialization, generation & erasing (EASTL list/vector)

- Particle bouncing

## What doesn't work

- ~~Debug camera crashes the game while the contrail hook is enabled~~ ExOpts was the culprit and was patched in the latest build! Make sure you update it!

- Fixed pipeline rendering - this will only work via pixel shaders. You will not be able to use this with, for example, a GeForce 2.

- Check the top of `dllmain.cpp` for more up-to-date info

## Usage

- If not installed already, install either [Widescreen Fix](https://github.com/ThirteenAG/WidescreenFixesPack/releases/tag/nfsmw) (recommended) or [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) (only if you don't want Widescreen Fix)

- Make sure you're running with the RELOADED No-CD 1.3 exe! (MD5: C0516B485065FABDD69579816B5DF763)

- Extract the .zip to the root directory of the game

- Reconfigure the .ini in the scripts folder to your liking

- Make sure that texture filtering is turned on in video settings! Otherwise sparks will be drawn at wrong coordinates!

- In case you're using NextGenGraphics.asi, please enable `UseD3DDeviceTexture` in the .ini
