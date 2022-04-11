# Setup/Installation

 - Use macOS (Should work on Windows since Qt is multiplatform, but haven't tested this) -- definitely works on macOS Monterey
 - Install Qt Creator. This may take a long time. I apologize. Here is the link (which hopefully works) for the Qt installer: https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4
 - Open Qt Creator
 - Click File in the menu bar
 - Click Open File or Project
 - Find the mm-prj1 folder (which contains the source code for this project) and select mm-prj1.pro and click Open

# Running the program

 1. Click the green play button (Run) in Qt Creator. 
 2. You should see a window open with two big buttons: WAV and PNG. Click one of them to get started.

 Note: For the PNG portion, you can select the kind of dithering technique you would like to be used: Bayer matrix or Floyd-Steinberg. Unfortunately, in order to switch between modes you must open the file up all over again (i.e., you should select your dithering mode prior to opening the file, as the dithered image will not refresh after reselecting the dithering mode, only after opening a file).