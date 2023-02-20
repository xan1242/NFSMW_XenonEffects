# NFS Most Wanted - XenonEffects port

This is an attempt to port and implement XenonEffect particle emitters to NFS Most Wanted PC version.

Currently this is an early prototype. It may cause instability, so use it at your own risk.

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

## What doesn't work

- ~~Debug camera crashes the game while the contrail hook is enabled~~ ExOpts was the culprit and will be patched in the latest build!

- Proper texture loading from TextureInfo - currently the texture is loaded outside

- Particle bouncing seems broken (maybe it's an unused parameter in MW)

- Check the top of `dllmain.cpp` for more up-to-date info

## Usage

- Extract NFSMW_XenonEffect.asi and .ini to the scripts folder

- Place MAIN.dds either in game root or the scripts folder

- Reconfigure the .ini to your liking
