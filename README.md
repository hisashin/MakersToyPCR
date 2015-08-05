NinjaPCR
============
![image](https://raw.github.com/hisashin/NinjaPCR/master/img/logo.png)

NinjaPCR is Opensource(GPLv3) DNA Amplifier,Thermocycler Implementation for [Polymerase Chain Reaction](http://en.wikipedia.org/wiki/Polymerase_chain_reaction) developed by [Toriningen Inc.](http://www.tori.st) ([Shingo Hisakawa](https://www.facebook.com/hisakawa) and [Mariko Hisakawa](https://www.facebook.com/maripo)) in Tokyo,Japan.<br />

[Photos](https://www.facebook.com/hisakawa/media_set?set=a.10151895843079481.663784480&type=3)
[![image](https://raw.github.com/hisashin/NinjaPCR/master/img/pcr_1000.png)](https://www.facebook.com/hisakawa/media_set?set=a.10151895843079481.663784480&type=3)

[3D CAD](https://fusion360.autodesk.com/projects/ninjapcr)<br />
[![image](https://raw.github.com/hisashin/NinjaPCR/master/img/AutodeskCapture.png)](https://fusion360.autodesk.com/projects/ninjapcr)

[BOM](https://docs.google.com/spreadsheets/d/1Eu-5YibDgaByIjIB5OE9yRJ3uC1ED0U484PYJb1YKqg/edit#gid=36)

Forked from [OpenPCR](https://github.com/jperfetto/OpenPCR).

Major changes from OpenPCR:<br />
01. Get smaller by using AC adapter<br />
02. Platform changed from AdobeAIR to [ChromeApp](https://chrome.google.com/webstore/detail/makerstoy-pcr/hoeafinlaiemkjnkakfbdpobhpicjbmb/details)<br />
03. Support [Multi OS (Windows,Mac,Linux,ChromeOS)](https://support.google.com/chrome/answer/95411?hl=en)<br />
04. LCD removed,Software enchanced (ex.Temperature graph)<br />
05. Software auto update for new features (ex.Mail alert scheduled)<br />
06. Reusable Arduino UNO (firmware not overwritten for AdobeAIR)<br />
07. Reusable [Motor Driver Carrier](http://www.pololu.com/product/1451) (Not soldered to PCB)<br />
08. High precision at both low/high temperature (Scheduled)<br />
09. Added RGB LED (Scheduled)<br />
10. You can order primer from app (Scheduled)<br />

Minor differences from OpenPCR:<br />
11. Cleared its Design Rule Check of PCB<br />
12. Simpler metal parts (drilling to bending)<br />
13. Unit is changed from inch to mm<br />

Note:<br />
NinjaPCR-131202.brd is the best PCB to try.
After that I added photocoupler to test high/low temperature modes but analog switch is right choice for that purpose.<br />
Pololu legacy motor driver VNH3SP30 is used. if you want to use latest one, you should change a little.<br />
There seems a inconsistency error on EAGLE but... maybe not so severe. I'll check later.'
