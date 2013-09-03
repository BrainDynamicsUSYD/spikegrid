SOURCE=params.fs arrayops.fs classify.fs 
COMPILER=fsharpc
MONOOPTS= --optimize=all
FSHARPOPTS=--optimize+ --nologo --checked- --define:FS --consolecolors-
all:analysis.exe classify.exe order.exe
order.exe: params.fs orderparam.fs
	${COMPILER} ${FSHARPOPTS} params.fs orderparam.fs
classify.exe: ${SOURCE}
	${COMPILER} --nowarn:51 -g  -r "System.Windows.Forms.dll" -r "System.Drawing.dll"  ${SOURCE} ${FSHARPOPTS}
analysis.exe: analysis.fs
	${COMPILER} ${FSHARPOPTS} analysis.fs
