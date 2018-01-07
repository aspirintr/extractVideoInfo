# extractVideoInfo
ExtractVideoInfo project aims to extract detailed compression information from a video such as motion vectors, quantization scales, coded tree structures, residuals, prediction errors etc. and provide a library to show those to the user. 

It currently supports Mpeg1,2, h264 and HEVC (H265) codecs. Extracted info is limited now, mostly only Motion vectors. However, the developed structure lets other users to get any other information from the codec by adding few lines of codes.

This is an open source project. We need people to contribute.

## Prerequisites and Installation
Follow this [file](https://docs.google.com/document/d/1JhjbeEjnyoMA81fCONBkKB67RoKaeoZ4VYfdGhpFM44/edit?usp=sharing) I will translate later. If you need, send a message.

## Structure of the library
In a nut shell: 
* It depends on FFmpeg library,
  * FFmpeg library is modified to provide the requested info
  * Requested info is fetched during decode process of FFmpeg,
  * The requested info is returned as a side data, which is already a feature of FFmpeg
* Info is taken from FFmpeg at the end of decoding of each frame
* Fetched info is recorded into SQlite database tables
* There is also a QT project which can read related infor form the database and overlays it on the video.
## Licence
I need to first understand the licences and then I will decide on the one most suitable.
