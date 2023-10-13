import os
import sys
import struct
import subprocess

import tkinter
import tkinter.filedialog
import tkinter.messagebox
import tkinter.scrolledtext


class ProgSetInfo:
	'''Information on a program set.'''
	def __init__(self,fromStr):
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.name = str(fromStr.read(textL), "utf-8")
		'''The name of the set.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.summary = str(fromStr.read(textL), "utf-8")
		'''A summary of the set.'''
		numProg = struct.unpack(">Q", fromStr.read(8))[0]
		self.progNames = []
		'''The names of the programs'''
		self.progSummaries = []
		'''Quick summaries of the programs'''
		for i in range(numProg):
			textL = struct.unpack(">Q", fromStr.read(8))[0]
			tmpName = str(fromStr.read(textL), "utf-8")
			textL = struct.unpack(">Q", fromStr.read(8))[0]
			tmpSummary = str(fromStr.read(textL), "utf-8")
			self.progNames.append(tmpName)
			self.progSummaries.append(tmpSummary)


class ArgInfo:
	'''Information on a command line argument.'''
	def __init__(self,fromStr):
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.name = str(fromStr.read(textL), "utf-8")
		'''The name of the argument.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.summary = str(fromStr.read(textL), "utf-8")
		'''A summary of the argument.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.description = str(fromStr.read(textL), "utf-8")
		'''A longform description of the argument.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.usage = str(fromStr.read(textL), "utf-8")
		'''An example usage of the argument.'''
		self.isPublic = fromStr.read(1)[0] != 0
		'''Whether this argument should be visible.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.mainFlavor = str(fromStr.read(textL), "utf-8")
		'''The type of argument this is.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.subFlavor = str(fromStr.read(textL), "utf-8")
		'''The sub-type.'''
		extL = struct.unpack(">Q", fromStr.read(8))[0]
		self.extras = fromStr.read(extL)
		'''The extra data.'''


class ProgramInfo:
	'''Information on a program.'''
	def __init__(self,fromStr):
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.name = str(fromStr.read(textL), "utf-8")
		'''The name of the set.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.summary = str(fromStr.read(textL), "utf-8")
		'''A summary of the set.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.description = str(fromStr.read(textL), "utf-8")
		'''A longform description of the program.'''
		textL = struct.unpack(">Q", fromStr.read(8))[0]
		self.usage = str(fromStr.read(textL), "utf-8")
		'''An example usage of the program.'''
		numArg = struct.unpack(">Q", fromStr.read(8))[0]
		theArgs = []
		for i in range(numArg):
			theArgs.append(ArgInfo(fromStr))
		self.arguments = theArgs
		'''The arguments this takes.'''


def getProgramInfos(progPath,subPName = None):
	'''
	Get info for a program.
	@param progPath: The path to the program, and any prefix arguments.
	@param subPName: The name of the subprogram of interest, or None if not a set.
	@return: The info.
	'''
	runArgs = progPath[:]
	if not (subPName is None):
		runArgs.append(subPName)
	runArgs.append("--help_argdump")
	curRunP = subprocess.Popen(runArgs,stdout=subprocess.PIPE,stdin=subprocess.DEVNULL)
	allArgs = ProgramInfo(curRunP.stdout)
	curRunP.stdout.close()
	if curRunP.wait() != 0:
		raise IOError("Problem getting argument info.")
	return allArgs


def getProgramSetInfos(progPath):
	'''
	Get info for a program set.
	@param progPath: The path to the program.
	@param subPName: The name of the subprogram of interest, or None if not a set.
	@return: The info.
	'''
	runArgs = progPath[:]
	runArgs.append("--help_argdump")
	curRunP = subprocess.Popen(runArgs,stdout=subprocess.PIPE,stdin=subprocess.DEVNULL)
	allArgs = ProgSetInfo(curRunP.stdout)
	curRunP.stdout.close()
	if curRunP.wait() != 0:
		raise IOError("Problem getting set info.")
	return allArgs


class ArgumentFlavor:
	def manpageSynopMang(self,argD):
		'''
		Make some text for an argument synopsis in a manpage.
		@param argD: The argument data.
		'''
		raise ValueError("Not implemented for argument type")
	def makeArgGUI(self,forGui,argD):
		'''
		Make a gui for an argument.
		@param forGui: The gui to add to.
		@param argD: The argument data.
		'''
		raise ValueError("Not implemented for argument type")
	def getGUIArgTexts(self,forGui,forData):
		'''
		Get the argument texts for a given set of arguments.
		@param forGui: The gui this is for.
		@param forData: The data this made in makeArgGUI.
		@return: A list of argument
		'''
		raise ValueError("Not implemented for argument type")


class ArgumentFlavorFlag(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR"
	def makeArgGUI(self,forGui,argD):
		theV = tkinter.IntVar()
		theCB = tkinter.Checkbutton(forGui.myCanvas, text=argD.name, variable=theV)
		theCB.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theV, theCB, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		if forData[1].get() != 0:
			toRet.append(forData[0].name)
		return toRet


class ArgumentFlavorEnum(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR"
	def makeArgGUI(self,forGui,argD):
		# see if the class is in the handled classes
		if not ("enum_class" in forGui.valueCache):
			forGui.valueCache["enum_class"] = {}
		classNameL = struct.unpack(">Q", argD.extras[0:8])[0]
		className = str(argD.extras[8:(classNameL+8)], "utf-8")
		if className in forGui.valueCache["enum_class"]:
			tgtStrVar = forGui.valueCache["enum_class"][className]
		else:
			tgtStrVar = tkinter.StringVar(forGui.master, argD.name)
			forGui.valueCache["enum_class"][className] = tgtStrVar
		# make the radio button
		theCB = tkinter.Radiobutton(forGui.myCanvas, text=argD.name, variable=tgtStrVar, value=argD.name)
		theCB.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, tgtStrVar, theCB, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		if forData[1].get() == forData[0].name:
			toRet.append(forData[0].name)
		return toRet


class ArgumentFlavorInt(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fI###\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initValue = struct.unpack(">q", argD.extras[0:8])[0]
		theBox.insert(0, repr(initValue))
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(forData[2].get().strip())
		return toRet


class ArgumentFlavorIntVec(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "[\\fB" + manSan(argD.name) + "\\fR \\fI###\\fR]*"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		theBox = tkinter.scrolledtext.ScrolledText(forGui.myCanvas, height = 4, width = 40)
		theBox.grid(column = 0, row = forGui.gridR, columnspan = 2, rowspan = 4)
		forGui.gridR = forGui.gridR + 4
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curRetOpts = forData[2].get('1.0', tkinter.END).split("\n")
		for cv in curRetOpts:
			if len(cv.strip()) == 0:
				continue
			toRet.append(forData[0].name)
			toRet.append(cv.strip())
		return toRet


class ArgumentFlavorFloat(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fI###.###\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initValue = struct.unpack(">d", argD.extras[0:8])[0]
		theBox.insert(0, repr(initValue))
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(forData[2].get().strip())
		return toRet


class ArgumentFlavorFloatVec(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "[\\fB" + manSan(argD.name) + "\\fR \\fI###.###\\fR]*"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		theBox = tkinter.scrolledtext.ScrolledText(forGui.myCanvas, height = 4, width = 40)
		theBox.grid(column = 0, row = forGui.gridR, columnspan = 2, rowspan = 4)
		forGui.gridR = forGui.gridR + 4
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curRetOpts = forData[2].get('1.0', tkinter.END).split("\n")
		for cv in curRetOpts:
			if len(cv.strip()) == 0:
				continue
			toRet.append(forData[0].name)
			toRet.append(cv.strip())
		return toRet


class ArgumentFlavorString(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fITEXT\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initLen = struct.unpack(">Q", argD.extras[0:8])[0]
		initValue = str(argD.extras[8:(initLen+8)], "utf-8")
		theBox.insert(0, initValue)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(curVal)
		return toRet


class ArgumentFlavorStringVec(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "[\\fB" + manSan(argD.name) + "\\fR \\fITEXT\\fR]*"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		theBox = tkinter.scrolledtext.ScrolledText(forGui.myCanvas, height = 4, width = 40)
		theBox.grid(column = 0, row = forGui.gridR, columnspan = 2, rowspan = 4)
		forGui.gridR = forGui.gridR + 4
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curRetOpts = forData[2].get('1.0', tkinter.END).split("\n")
		for cv in curRetOpts:
			if len(cv.strip()) == 0:
				continue
			toRet.append(forData[0].name)
			toRet.append(cv.strip())
		return toRet


def parseFileExtensions(forExtra):
	numExts = struct.unpack(">Q",forExtra[0:8])[0]
	extBytes = forExtra[8:]
	allExts = []
	for i in range(numExts):
		curELen = struct.unpack(">Q", extBytes[0:8])[0]
		curETxt = str(extBytes[8:(8+curELen)],"utf-8")
		extBytes = extBytes[8+curELen:]
		allExts.append(curETxt)
	return (allExts, extBytes)


class ArgumentFlavorStringFileRead(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fIFILE\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initLen = struct.unpack(">Q", argD.extras[0:8])[0]
		initValue = str(argD.extras[8:(initLen+8)], "utf-8")
		theBox.insert(0, initValue)
		allExts = parseFileExtensions(argD.extras[initLen+8:])[0]
		theBtn = None
		if len(allExts) > 0:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.askopenfilename(title = argD.name, filetypes = tuple([(cv, "*" + cv) for cv in allExts]) )))
		else:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.askopenfilename(title = argD.name )))
		theBtn.grid(column = 2, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(curVal)
		return toRet


class ArgumentFlavorStringVecFileRead(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "[\\fB" + manSan(argD.name) + "\\fR \\fIFILE\\fR]*"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		theBox = tkinter.scrolledtext.ScrolledText(forGui.myCanvas, height = 4, width = 40)
		theBox.grid(column = 1, row = forGui.gridR, columnspan = 2, rowspan = 4)
		forGui.gridR = forGui.gridR + 4
		allExts = parseFileExtensions(argD.extras)[0]
		theBtn = None
		if len(allExts) > 0:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateMultiFileName(theBox, tkinter.filedialog.askopenfilename(title = argD.name, filetypes = tuple([(cv, "*" + cv) for cv in allExts]) )))
		else:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateMultiFileName(theBox, tkinter.filedialog.askopenfilename(title = argD.name )))
		theBtn.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curRetOpts = forData[2].get('1.0', tkinter.END).split("\n")
		for cv in curRetOpts:
			if len(cv.strip()) == 0:
				continue
			toRet.append(forData[0].name)
			toRet.append(cv.strip())
		return toRet


class ArgumentFlavorStringFileWrite(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fIFILE\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initLen = struct.unpack(">Q", argD.extras[0:8])[0]
		initValue = str(argD.extras[8:(initLen+8)], "utf-8")
		theBox.insert(0, initValue)
		allExts = parseFileExtensions(argD.extras[initLen+8:])[0]
		theBtn = None
		if len(allExts) > 0:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.asksaveasfilename(title = argD.name, filetypes = tuple([(cv, "*" + cv) for cv in allExts]), defaultextension="*.*" )))
		else:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.asksaveasfilename(title = argD.name )))
		theBtn.grid(column = 2, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(curVal)
		return toRet


class ArgumentFlavorStringVecFileWrite(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "[\\fB" + manSan(argD.name) + "\\fR \\fIFILE\\fR]*"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		theBox = tkinter.scrolledtext.ScrolledText(forGui.myCanvas, height = 4, width = 40)
		theBox.grid(column = 1, row = forGui.gridR, columnspan = 2, rowspan = 4)
		forGui.gridR = forGui.gridR + 4
		allExts = parseFileExtensions(argD.extras)[0]
		theBtn = None
		if len(allExts) > 0:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateMultiFileName(theBox, tkinter.filedialog.asksaveasfilename(title = argD.name, filetypes = tuple([(cv, "*" + cv) for cv in allExts]), defaultextension="*.*" )))
		else:
			theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateMultiFileName(theBox, tkinter.filedialog.asksaveasfilename(title = argD.name )))
		theBtn.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curRetOpts = forData[2].get('1.0', tkinter.END).split("\n")
		for cv in curRetOpts:
			if len(cv.strip()) == 0:
				continue
			toRet.append(forData[0].name)
			toRet.append(cv.strip())
		return toRet


class ArgumentFlavorStringFolderRead(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fIDIR\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initLen = struct.unpack(">Q", argD.extras[0:8])[0]
		initValue = str(argD.extras[8:(initLen+8)], "utf-8")
		theBox.insert(0, initValue)
		theBtn = None
		theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.askdirectory(title = argD.name, mustexist=True)))
		theBtn.grid(column = 2, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(curVal)
		return toRet


class ArgumentFlavorStringFolderWrite(ArgumentFlavor):
	def manpageSynopMang(self,argD):
		return "\\fB" + manSan(argD.name) + "\\fR \\fIDIR\\fR"
	def makeArgGUI(self,forGui,argD):
		theNam = tkinter.Label(forGui.myCanvas, text = argD.name)
		theNam.grid(column = 0, row = forGui.gridR)
		theBox = tkinter.Entry(forGui.myCanvas)
		theBox.grid(column = 1, row = forGui.gridR)
		theBox.delete(0, tkinter.END)
		initLen = struct.unpack(">Q", argD.extras[0:8])[0]
		initValue = str(argD.extras[8:(initLen+8)], "utf-8")
		theBox.insert(0, initValue)
		theBtn = None
		theBtn = tkinter.Button(forGui.myCanvas, text='Browse', command = lambda: forGui.updateSingleFileName(theBox, tkinter.filedialog.askdirectory(title = argD.name, mustexist=False)))
		theBtn.grid(column = 2, row = forGui.gridR)
		theLab = tkinter.Label(forGui.myCanvas, text = argD.summary)
		theLab.grid(column = 3, row = forGui.gridR)
		forGui.gridR = forGui.gridR + 1
		forGui.argPassFlavs.append(self)
		forGui.argPassCaps.append([argD, theNam, theBox, theBtn, theLab])
	def getGUIArgTexts(self,forGui,forData):
		toRet = []
		curVal = forData[2].get().strip()
		if len(curVal) > 0:
			toRet.append(forData[0].name)
			toRet.append(curVal)
		return toRet


argFlavorMap = {}
argFlavorMap[("flag","")] = ArgumentFlavorFlag()
argFlavorMap[("enum","")] = ArgumentFlavorEnum()
argFlavorMap[("int","")] = ArgumentFlavorInt()
argFlavorMap[("intvec","")] = ArgumentFlavorIntVec()
argFlavorMap[("float","")] = ArgumentFlavorFloat()
argFlavorMap[("floatvec","")] = ArgumentFlavorFloatVec()
argFlavorMap[("string","")] = ArgumentFlavorString()
argFlavorMap[("stringvec","")] = ArgumentFlavorStringVec()
argFlavorMap[("string","fileread")] = ArgumentFlavorStringFileRead()
argFlavorMap[("stringvec","fileread")] = ArgumentFlavorStringVecFileRead()
argFlavorMap[("string","filewrite")] = ArgumentFlavorStringFileWrite()
argFlavorMap[("stringvec","filewrite")] = ArgumentFlavorStringVecFileWrite()
argFlavorMap[("string","folderread")] = ArgumentFlavorStringFolderRead()
argFlavorMap[("string","folderwrite")] = ArgumentFlavorStringFolderWrite()


class ArgumentGUIFrame(tkinter.Frame):
	'''Let the user pick arguments for a program.'''
	def updateSingleFileName(self, forBox, newName):
		if newName:
			forBox.delete(0, tkinter.END)
			forBox.insert(0, newName)
	def updateMultiFileName(self, forBox, newName):
		if newName:
			forBox.insert(tkinter.END, '\\n' + newName)
	def __init__(self,forProg,checkPref,master=None):
		super().__init__(master)
		self.master = master
		self.finalArgs = None
		'''The final arguments: only set on OK.'''
		self.progCheckPref = checkPref
		'''The way to run the program: used to check arguments.'''
		self.mainProg = forProg
		'''The program this is for.'''
		self.argPassFlavs = []
		'''The flavors of the arguments this works with'''
		self.argPassCaps = []
		'''Extra data for the arguments this can work with.'''
		self.myCanvas = tkinter.Canvas(self)
		'''The main canvas of this thing'''
		self.gridR = 0
		'''The grid row this is on.'''
		self.valueCache = {}
		'''Named things to refer back to.'''
		# add a summary line
		self.sumLine = tkinter.Label(self.myCanvas, text = forProg.summary)
		self.sumLine.grid(column = 0, row = self.gridR, columnspan=3)
		self.gridR = self.gridR + 1
		# make stuff for the arguments
		for i in range(len(forProg.arguments)):
			carg = forProg.arguments[i]
			if not carg.isPublic:
				continue
			argTp = (carg.mainFlavor, carg.subFlavor)
			if not (argTp in argFlavorMap):
				continue
			argFlav = argFlavorMap[argTp]
			argFlav.makeArgGUI(self,carg)
		# make the ok and cancel buttons
		self.canBtn = tkinter.Button(self.myCanvas, text = 'Cancel', command = lambda: self.actionCancel())
		self.canBtn.grid(column = 0, row = self.gridR)
		self.goBtn = tkinter.Button(self.myCanvas, text = 'OK', command = lambda: self.actionGo())
		self.goBtn.grid(column = 1, row = self.gridR)
		# make a scroll bar
		#TODO fixme
		self.myCanvas.pack(side=tkinter.LEFT)
		mainscr = tkinter.Scrollbar(self, command=self.myCanvas.yview)
		mainscr.pack(side=tkinter.LEFT, fill='y')
		self.myCanvas.configure(yscrollcommand = mainscr.set)
		self.pack()
	def actionCancel(self):
		self.master.destroy()
	def actionGo(self):
		retArgs = []
		# get the arguments
		for i in range(len(self.argPassFlavs)):
			curGArgs = self.argPassFlavs[i].getGUIArgTexts(self,self.argPassCaps[i])
			retArgs.extend(curGArgs)
		# see if the program likes them
		curRunP = subprocess.Popen(self.progCheckPref + ["--help_id10t"] + retArgs,stderr=subprocess.PIPE,stdout=subprocess.DEVNULL,stdin=subprocess.DEVNULL)
		allErrT = curRunP.stderr.read()
		curRunP.stderr.close()
		retCode = curRunP.wait()
		if (len(allErrT) > 0) or (retCode != 0):
			tkinter.messagebox.showerror(title="Problem with arguments!", message=str(allErrT,"utf-8"))
			return
		# all good, set and quit
		self.finalArgs = retArgs
		self.master.destroy()


class ProgramSelectGUIFrame(tkinter.Frame):
	'''Pick a program to run.'''
	def __init__(self,forSet,master=None):
		super().__init__(master)
		self.master = master
		self.finalPI = None
		'''The final selection: only set on OK.'''
		self.myCanvas = tkinter.Canvas(self)
		'''The main canvas of this thing'''
		# make the selections
		gridR = 0
		self.progButs = []
		self.progTexts = []
		for i in range(len(forSet.progNames)):
			progBut = tkinter.Button(self.myCanvas, text = forSet.progNames[i], command = self.helpMakeGo(i))
			progBut.grid(column = 0, row = gridR)
			progTxt = tkinter.Label(self.myCanvas, text = forSet.progSummaries[i])
			progTxt.grid(column = 1, row = gridR)
			self.progButs.append(progBut)
			self.progTexts.append(progTxt)
			gridR = gridR + 1
		# make the cancel button
		self.canBtn = tkinter.Button(self.myCanvas, text = 'Cancel', command = lambda: self.actionCancel())
		self.canBtn.grid(column = 0, row = gridR)
		# make a scroll bar
		self.myCanvas.pack(side=tkinter.LEFT)
		mainscr = tkinter.Scrollbar(self, command=self.myCanvas.yview)
		mainscr.pack(side=tkinter.LEFT, fill='y')
		self.myCanvas.configure(yscrollcommand = mainscr.set)
		self.pack()
	def actionCancel(self):
		self.master.destroy()
	def actionGo(self,forProgI):
		self.finalPI = forProgI
		self.master.destroy()
	def helpMakeGo(self,forInd):
		def doIt():
			self.actionGo(forInd)
		return doIt


def htmlSan(theStr):
	'''
	Make a string safe for html.
	@param theStr: The string to make safe.
	@return: The safe string.
	'''
	outStr = theStr
	outStr = outStr.replace("&","&amp;")
	outStr = outStr.replace('"',"&quot;")
	outStr = outStr.replace("'","&apos;")
	outStr = outStr.replace("<","&lt;")
	outStr = outStr.replace(">","&gt;")
	return outStr


def writeHTMLForProg(progData,toStr):
	'''
	Write an HTML section for a program.
	@param progData: The parsed data for the program.
	@param toStr: The stream to write to.
	'''
	toStr.write("<P>" + htmlSan(progData.usage) + "</P>\n")
	if len(progData.description) > 0:
		allDesc = [cv.strip() for cv in progData.description.split("\n")]
		for cdesc in allDesc:
			toStr.write("<P>" + htmlSan(cdesc) + "</P>\n")
	else:
		toStr.write("<P>" + htmlSan(progData.summary) + "</P>\n")
	toStr.write("<TABLE>\n")
	for carg in progData.arguments:
		if not carg.isPublic:
			continue
		toStr.write("	<TR>\n")
		toStr.write("		<TD>" + htmlSan(carg.name) + "</TD>\n")
		toStr.write("		<TD>" + htmlSan(carg.summary) + "</TD>\n")
		toStr.write("	</TR>\n")
		if len(carg.description) > 0:
			toStr.write("	<TR>\n")
			toStr.write("		<TD></TD>\n")
			allDesc = [cv.strip() for cv in carg.description.split("\n")]
			toStr.write("		<TD>\n")
			for cdesc in allDesc:
				toStr.write("			<P>" + htmlSan(cdesc) + "</P>\n")
			toStr.write("		</TD>\n")
			toStr.write("	</TR>\n")
		if len(carg.usage) > 0:
			toStr.write("	<TR>\n")
			toStr.write("		<TD></TD>\n")
			toStr.write("		<TD>" + htmlSan(carg.usage) + "</TD>\n")
			toStr.write("	</TR>\n")
	toStr.write("</TABLE>\n")


def writeHTMLProgram(progPath,toStr):
	'''
	Write HTML documentation for a program.
	@param progPath: The path to the program, and any forced arguments.
	@param toStr: The stream to write to.
	'''
	progD = getProgramInfos(progPath,None)
	toStr.write("<!DOCTYPE html>\n")
	toStr.write("<HTML>\n")
	toStr.write("<HEAD>\n")
	toStr.write("	<TITLE>" + htmlSan(progD.name) + "</TITLE>\n")
	toStr.write("</HEAD>\n")
	toStr.write("<BODY>\n")
	toStr.write("<P>" + htmlSan(progD.name) + "</P>\n")
	writeHTMLForProg(progD,toStr)
	toStr.write("</BODY>\n")
	toStr.write("</HTML>\n")


def writeHTMLProgramSet(progPath,toStr):
	'''
	Write HTML documentation for a program set.
	@param progPath: The path to the program, and any forced arguments.
	@param toStr: The stream to write to.
	'''
	progS = getProgramSetInfos(progPath)
	toStr.write("<!DOCTYPE html>\n")
	toStr.write("<HTML>\n")
	toStr.write("<HEAD>\n")
	toStr.write("	<TITLE>" + htmlSan(progS.name) + "</TITLE>\n")
	toStr.write("</HEAD>\n")
	toStr.write("<BODY>\n")
	toStr.write("<P>" + htmlSan(progS.name) + "</P>\n")
	toStr.write("<P>" + htmlSan(progS.summary) + "</P>\n")
	toStr.write("<TABLE>\n")
	allProgDs = []
	for i in range(len(progS.progNames)):
		curNam = progS.progNames[i]
		curSum = progS.progSummaries[i]
		curProD = getProgramInfos(progPath,curNam)
		allProgDs.append(curProD)
		toStr.write("	<TR>\n")
		toStr.write("		<TD><A HREF=\"#prog" + repr(i) + "\">" + htmlSan(curNam) + "</A></TD>\n")
		toStr.write("		<TD>" + htmlSan(curSum) + "</TD>\n")
		toStr.write("	</TR>\n")
	toStr.write("</TABLE>\n")
	for i in range(len(progS.progNames)):
		toStr.write("<HR>\n")
		toStr.write("<P><A ID=\"prog" + repr(i) + "\">" + htmlSan(progS.name) + " " + htmlSan(progS.progNames[i]) + "</A></P>\n")
		writeHTMLForProg(allProgDs[i],toStr)
	toStr.write("</BODY>\n")
	toStr.write("</HTML>\n")


def manSan(theStr):
	'''
	Sanitize a string for a manpage.
	'''
	outStr = theStr
	outStr = outStr.replace("-", "\\-")
	return outStr


def writeManpageForProg(psetName,progData,toStr):
	'''
	Write a manpage for a program.
	@param psetName: The name of the program set it is in, if any.
	@param progData: The parsed data for the program.
	@param toStr: The stream to write to.
	'''
	workPName = progData.name
	if len(psetName) > 0:
		workPName = psetName + " " + workPName
		toStr.write(".TH " + manSan(psetName).upper() + "-" + manSan(progData.name).upper() + " 1\n")
	else:
		toStr.write(".TH " + manSan(progData.name).upper() + " 1\n")
	toStr.write(".SH NAME\n")
	toStr.write(manSan(workPName) + " \\- " + manSan(progData.summary) + "\n")
	toStr.write(".SH SYNOPSIS\n")
	toStr.write(".B " + manSan(workPName) + "\n")
	for carg in progData.arguments:
		if not carg.isPublic:
			continue
		argTp = (carg.mainFlavor, carg.subFlavor)
		if not (argTp in argFlavorMap):
			continue
		argFlav = argFlavorMap[argTp]
		toStr.write(argFlav.manpageSynopMang(carg) + "\n")
	toStr.write(".SH DESCRIPTION\n")
	toStr.write(".B " + manSan(workPName) + "\n")
	if len(progData.description) > 0:
		allDesc = [cv.strip() for cv in progData.description.split("\n")]
		for cdesc in allDesc:
			toStr.write(manSan(cdesc) + "\n")
	else:
		toStr.write(manSan(progData.summary) + "\n")
	toStr.write(".SH OPTIONS\n")
	for carg in progData.arguments:
		argTp = (carg.mainFlavor, carg.subFlavor)
		if not (argTp in argFlavorMap):
			continue
		argFlav = argFlavorMap[argTp]
		toStr.write(".TP\n")
		toStr.write(argFlav.manpageSynopMang(carg) + "\n")
		toStr.write(manSan(carg.summary) + "\n")


def writeManpageProgram(progPath,toStr):
	'''
	Write manpage documentation for a program.
	@param progPath: The path to the program.
	@param subPName: The sub-name of the program of interest.
	@param toStr: The stream to write to.
	'''
	progD = getProgramInfos(progPath,None)
	writeManpageForProg("",progD,toStr)


def writeManpageProgramSet(progPath,toFold,prefix):
	'''
	Write HTML documentation for a program set.
	@param progPath: The path to the program.
	@param toFold: The folder to write to.
	@param prefix: The prefix for the file names.
	'''
	progS = getProgramSetInfos(progPath)
	toStr = open(os.path.join(toFold,prefix + ".1"), "w")
	toStr.write(".TH " + manSan(progS.name).upper() + " 1\n")
	toStr.write(".SH NAME\n")
	toStr.write(manSan(progS.name) + " \\- " + manSan(progS.summary) + "\n")
	toStr.write(".SH SYNOPSIS\n")
	toStr.write(".B " + manSan(progS.name) + "\n")
	toStr.write("\\fIPROGRAM\\fR \\fIARGS\\fR ...\n")
	toStr.write(".SH DESCRIPTION\n")
	toStr.write(".B " + manSan(progS.name) + "\n")
	toStr.write(manSan(progS.summary) + "\n")
	toStr.write(".SH OPTIONS\n")
	allProgDs = []
	for i in range(len(progS.progNames)):
		curNam = progS.progNames[i]
		curSum = progS.progSummaries[i]
		curProD = getProgramInfos(progPath,curNam)
		allProgDs.append(curProD)
		toStr.write(".TP\n")
		toStr.write("\\fB" + manSan(progS.name) + "\\fR " + "\\fB" + manSan(curNam) + "\\fR \\fIARGS\\fR ...\n")
		toStr.write(manSan(curSum) + "\n")
	toStr.close()
	for i in range(len(allProgDs)):
		subStr = open(os.path.join(toFold,prefix + "-" + progS.progNames[i] + ".1"), "w")
		writeManpageForProg(progS.name,allProgDs[i],subStr)
		subStr.close()


def runGuiProgram(progPath,subPName = None):
	'''
	Run a gui for a specific program.
	@param progPath: The path to the program.
	@param subPName: The name of the sub-program to run, if relevant.
	'''
	progD = getProgramInfos(progPath,subPName)
	# get the arguments
	root = tkinter.Tk()
	root.title(progD.name)
	checkPref = progPath[:]
	if not (subPName is None):
		checkPref.append(subPName)
	app = ArgumentGUIFrame(progD, checkPref, root)
	app.mainloop()
	if app.finalArgs is None:
		return
	fullCallAs = progPath[:]
	if not (subPName is None):
		fullCallAs.append(subPName)
	fullCallAs.extend(app.finalArgs)
	print(" ".join(fullCallAs))
	# run the thing
	curRunP = subprocess.Popen(fullCallAs,stdout=subprocess.DEVNULL,stdin=subprocess.DEVNULL,stderr=subprocess.PIPE)
	allErrT = curRunP.stderr.read()
	allErrT = str(allErrT, "utf-8").strip()
	curRunP.stderr.close()
	if (curRunP.wait() != 0) or (len(allErrT) != 0):
		tkinter.messagebox.showerror(title="Problem running program!", message=allErrT)
		raise IOError("Problem running program.")


def runGuiProgramSet(progPath):
	'''
	Run a gui for a program set.
	@param progPath: The path to the program.
	'''
	progS = getProgramSetInfos(progPath)
	# select the thing to run
	root = tkinter.Tk()
	root.title(progS.name)
	app = ProgramSelectGUIFrame(progS, root)
	app.mainloop()
	if app.finalPI is None:
		return
	# select the arguments for the thing
	runGuiProgram(progPath, progS.progNames[app.finalPI])


if __name__ == "__main__":
	if len(sys.argv) < 2:
		raise ValueError("No action provided.")
	actName = sys.argv[1]
	if (actName == "--help") or (actName == "-h") or (actName == "/?"):
		print("Mangle arguments for a program (set).")
		print("htmlp path/to/program EXTRA_ARGS")
		print("    Document a program on stdout.")
		print("htmls path/to/program EXTRA_ARGS")
		print("    Document a program set on stdout.")
		print("manp path/to/program EXTRA_ARGS")
		print("    Make manpage for a program.")
		print("mans path/to/program EXTRA_ARGS folder/path prefix")
		print("    Make manpage for a program set.")
		print("guip path/to/program EXTRA_ARGS")
		print("    Run a gui for a program.")
		print("guis path/to/program EXTRA_ARGS")
		print("    Run a gui for a program set.")
	elif actName == "--version":
		print("ArgMang 0.0")
		print("Copyright (C) 2022 Benjamin Crysup")
		print("License LGPLv3: Lesser GNU GPL version 3")
		print("This is free software: you are free to change and redistribute it.")
		print("There is NO WARRANTY, to the extent permitted by law.")
	elif actName == "htmlp":
		if(len(sys.argv)<3):
			raise ValueError("htmlp needs the program to get data for")
		progExe = sys.argv[2:]
		writeHTMLProgram(progExe,sys.stdout)
	elif actName == "htmls":
		if(len(sys.argv)<3):
			raise ValueError("htmls needs the program set to get data for")
		progExe = sys.argv[2:]
		writeHTMLProgramSet(progExe,sys.stdout)
	elif actName == "manp":
		if(len(sys.argv)<3):
			raise ValueError("manp needs the program set to get data for")
		progExe = sys.argv[2:]
		writeManpageProgram(progExe,sys.stdout)
	elif actName == "mans":
		if len(sys.argv) < 5:
			raise ValueError("mans needs the program to work with, the output folder and output prefix")
		progExe = sys.argv[2:-2]
		writeManpageProgramSet(progExe,sys.argv[-2],sys.argv[-1])
	elif actName == "guip":
		if(len(sys.argv)<3):
			raise ValueError("guip needs the program to manage")
		progExe = sys.argv[2:]
		runGuiProgram(progExe,None)
	elif actName == "guis":
		if(len(sys.argv)<3):
			raise ValueError("guis needs the program set to manage")
		progExe = sys.argv[2:]
		runGuiProgramSet(progExe)
	else:
		raise ValueError("Unknown action: " + actName)




