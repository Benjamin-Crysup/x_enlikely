'''Code for parsing/documenting arguments.'''

import sys
import struct
import traceback

class ArgumentOption:
	'''An argument to a program.'''
	def __init__(self):
		'''
		Set up the common stuff.
		'''
		self.isCommon = True
		'''Whether this option should be advertised.'''
		self.name = ""
		'''The reporting name of this option.'''
		self.summary = ""
		'''The one-line summary of this option.'''
		self.description = ""
		'''A full description of this option, or empty if the summary is enough.'''
		self.usage = ""
		'''An example of how to use the option.'''
		self.typeCode = ""
		'''The primary type of this option.'''
		self.extTypeCode = ""
		'''The secondary type of this option.'''
	def canParse(self, forArgs):
		'''
		Get whether this can parse the first argument in a set.
		@param forArgs: The arguments to consider parsing.
		@return: Whether this is a relevant argument.
		'''
		raise NotImplementedError("Use a subclass.")
	def parse(self, forArgs, forProg):
		'''
		Parse some arguments.
		@param forArgs: The arguments to consider parsing.
		@param forProg: The program this is parsing for.
		@return: The remaining arguments.
		'''
		raise NotImplementedError("Use a subclass.")
	def idiotCheck(self):
		'''
		Perform any special checks on an argument.
		'''
		return
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		self.dumpCommonInfo(toStr,0)
	def dumpCommonInfo(self,toStr,numExtra):
		'''
		Dump the common info on this thing.
		@param toStr: The stream to write to.
		@param numExtra: The number of extra bytes that come after.
		'''
		# name
		curD = bytes(self.name,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# summary
		curD = bytes(self.summary,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# description
		curD = bytes(self.description,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# usage
		curD = bytes(self.usage,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# common
		toStr.write(bytes([1]) if self.isCommon else bytes([0]))
		# type codes
		curD = bytes(self.typeCode,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		curD = bytes(self.extTypeCode,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# how much extra crap
		toStr.write(struct.pack(">Q",numExtra))


class ArgumentOptionHelp(ArgumentOption):
	'''Look for help requests.'''
	def __init__(self):
		'''
		Set up a help option.
		'''
		ArgumentOption.__init__(self)
		self.isCommon = False
		self.name = "--help"
		self.summary = "Print out help information."
		self.typeCode = "meta"
		self.extTypeCode = ""
	def canParse(self, forArgs):
		return (forArgs[0] == "--help") or (forArgs[0] == "-h") or (forArgs[0] == "/?")
	def parse(self, forArgs, forProg):
		forProg.needRun = False
		forProg.needIdiot = False
		forProg.printHelp(forProg.useOut)
		return []


class ArgumentOptionVersion(ArgumentOption):
	'''Look for version info requests.'''
	def __init__(self):
		'''
		Set up a version option.
		'''
		ArgumentOption.__init__(self)
		self.isCommon = False
		self.name = "--version"
		self.summary = "Print out version information."
		self.typeCode = "meta"
		self.extTypeCode = ""
	def canParse(self, forArgs):
		return (forArgs[0] == "--version")
	def parse(self, forArgs, forProg):
		forProg.needRun = False
		forProg.needIdiot = False
		forProg.printVersion(forProg.useOut)
		return []


class ArgumentOptionArgdump(ArgumentOption):
	'''Look for help requests.'''
	def __init__(self):
		'''
		Set up a help option.
		'''
		ArgumentOption.__init__(self)
		self.isCommon = False
		self.name = "--help_argdump"
		self.summary = "Dump out argument information."
		self.typeCode = "meta"
		self.extTypeCode = ""
	def canParse(self, forArgs):
		return (forArgs[0] == "--help_argdump")
	def parse(self, forArgs, forProg):
		forProg.needRun = False
		forProg.needIdiot = False
		forProg.dumpArguments(forProg.useOut)
		return []


class ArgumentOptionIdiot(ArgumentOption):
	'''Look for version info requests.'''
	def __init__(self):
		'''
		Set up a version option.
		'''
		ArgumentOption.__init__(self)
		self.isCommon = False
		self.name = "--help_id10t"
		self.summary = "Do not actually run, just check arguments."
		self.typeCode = "meta"
		self.extTypeCode = ""
	def canParse(self, forArgs):
		return (forArgs[0] == "--help_id10t")
	def parse(self, forArgs, forProg):
		forProg.needRun = False
		return forArgs[1:]


class ArgumentOptionNamed(ArgumentOption):
	'''A simple named option.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOption.__init__(self)
		self.name = useName
	def canParse(self, forArgs):
		return (forArgs[0] == self.name)


class ArgumentOptionFlag(ArgumentOptionNamed):
	'''A flag.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "flag"
		self.extTypeCode = ""
		self.value = False
		'''The parsed value of this option.'''
	def parse(self, forArgs, forProg):
		self.value = True
		return forArgs[1:]


class ArgumentOptionEnumValue:
	'''Storage for the current value of an enum option.'''
	def __init__(self, className):
		'''
		Set up the value.
		@param className: The name of this class. Should be unique.
		'''
		self.className = className
		'''The name of this value.'''
		self.value = 0
		'''The current value.'''
		self.numRegistered = 0
		'''The number of things registered.'''


class ArgumentOptionEnum(ArgumentOptionNamed):
	'''Set an enum value.'''
	def __init__(self,useName,forValue):
		'''
		Set up an option.
		@param useName: The name of this option.
		@param forValue: The value to work over.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "enum"
		self.extTypeCode = ""
		self.myClass = forValue
		'''The value this is for.'''
		self.setValue = forValue.numRegistered
		'''The value this should set the enum to.'''
		forValue.numRegistered = forValue.numRegistered + 1
	def parse(self, forArgs, forProg):
		self.myClass.value = self.setValue
		return forArgs[1:]
	def value(self):
		'''
		Get whether this is the selected enum.
		@return: Whether this is hot.
		'''
		return (self.myClass.value == self.setValue)
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		curD = bytes(self.myClass.className, "utf-8")
		self.dumpCommonInfo(toStr,8 + len(curD) + 1)
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		toStr.write(bytes([1]) if (self.setValue == 0) else bytes([0]))


class ArgumentOptionInteger(ArgumentOptionNamed):
	'''Set an integer value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "int"
		self.extTypeCode = ""
		self.value = 0
		'''The value of the integer.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value = int(forArgs[1])
		return forArgs[2:]
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		self.dumpCommonInfo(toStr,8)
		toStr.write(struct.pack(">Q",self.value))


class ArgumentOptionFloat(ArgumentOptionNamed):
	'''Set a float value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "float"
		self.extTypeCode = ""
		self.value = 0.0
		'''The value of the float.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value = float(forArgs[1])
		return forArgs[2:]
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		self.dumpCommonInfo(toStr,8)
		toStr.write(struct.pack(">d",self.value))


class ArgumentOptionString(ArgumentOptionNamed):
	'''Set a string value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "string"
		self.extTypeCode = ""
		self.value = ""
		'''The value of the string.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value = forArgs[1]
		return forArgs[2:]
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		curD = bytes(self.value, "utf-8")
		self.dumpCommonInfo(toStr,8 + len(curD))
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)


class ArgumentOptionIntegerVector(ArgumentOptionNamed):
	'''Set an integer value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "intvec"
		self.extTypeCode = ""
		self.value = []
		'''The value of the integer.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value.append(int(forArgs[1]))
		return forArgs[2:]


class ArgumentOptionFloatVector(ArgumentOptionNamed):
	'''Set a float value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "floatvec"
		self.extTypeCode = ""
		self.value = []
		'''The value of the float.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value.append(float(forArgs[1]))
		return forArgs[2:]


class ArgumentOptionStringVector(ArgumentOptionNamed):
	'''Set a string value.'''
	def __init__(self,useName):
		'''
		Set up an option.
		@param useName: The name of this option.
		'''
		ArgumentOptionNamed.__init__(self, useName)
		self.typeCode = "stringvec"
		self.extTypeCode = ""
		self.value = []
		'''The value of the string.'''
	def parse(self, forArgs, forProg):
		if len(forArgs) < 2:
			raise IndexError("No value provided for " + self.name)
		self.value.append(forArgs[1])
		return forArgs[2:]


class ArgumentOptionThreadcount(ArgumentOptionInteger):
	'''The number of threads to spin up.'''
	def __init__(self):
		'''
		Set up a threadcount.
		'''
		ArgumentOptionInteger.__init__(self,"--thread")
		self.value = 1
		self.summary = "The number of threads to spin up."
		self.usage = "--thread 1"
	def idiotCheck(self):
		if self.value <= 0:
			raise ValueError("Need at least one thread.")


class ArgumentOptionThreadgrain(ArgumentOptionInteger):
	'''The number of threads to spin up.'''
	def __init__(self):
		'''
		Set up a threadcount.
		'''
		ArgumentOptionInteger.__init__(self,"--threadgrain")
		self.value = 65536
		self.summary = "The number of things to do per thread."
		self.usage = "--threadgrain 65536"
	def idiotCheck(self):
		if self.value <= 0:
			raise ValueError("Each thread needs to do at least one thing.")


def _dumpFileIOItems(dumpFor, toStr, hasCur):
	# prepare to dump
	extDs = []
	totNumExtD = 8
	for ext in dumpFor.validExts:
		curED = bytes(ext, "utf-8")
		extDs.append(curED)
		totNumExtD += (8 + len(curED))
	if hasCur:
		curD = bytes(dumpFor.value, "utf-8")
		totNumExtD += (8 + len(curD))
	# common
	dumpFor.dumpCommonInfo(toStr,totNumExtD)
	# current value
	if hasCur:
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
	# number of extensions
	toStr.write(struct.pack(">Q",len(dumpFor.validExts)))
	# extensions
	for curED in extDs:
		toStr.write(struct.pack(">Q",len(curED)))
		toStr.write(curED)


class ArgumentOptionFileRead(ArgumentOptionString):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a file read option.
		@param useName: The name of this option.
		'''
		ArgumentOptionString.__init__(self,useName)
		self.extTypeCode = "fileread"
		self.validExts = set()
		'''The valid extensions for this option.'''
		self.required = False
		'''Whether this option is required.'''
	def idiotCheck(self):
		if self.required and (len(self.value) == 0):
			raise ValueError("Missing value for " + self.name)
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		_dumpFileIOItems(self, toStr, True)


class ArgumentOptionFileWrite(ArgumentOptionString):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a file write option.
		@param useName: The name of this option.
		'''
		ArgumentOptionString.__init__(self,useName)
		self.extTypeCode = "filewrite"
		self.validExts = set()
		'''The valid extensions for this option.'''
		self.required = False
		'''Whether this option is required.'''
	def idiotCheck(self):
		if self.required and (len(self.value) == 0):
			raise ValueError("Missing value for " + self.name)
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		_dumpFileIOItems(self, toStr, True)


class ArgumentOptionFileReadVector(ArgumentOptionStringVector):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a file read option.
		@param useName: The name of this option.
		'''
		ArgumentOptionStringVector.__init__(self,useName)
		self.extTypeCode = "fileread"
		self.validExts = set()
		'''The valid extensions for this option.'''
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		_dumpFileIOItems(self, toStr, False)


class ArgumentOptionFileWriteVector(ArgumentOptionStringVector):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a file read option.
		@param useName: The name of this option.
		'''
		ArgumentOptionStringVector.__init__(self,useName)
		self.extTypeCode = "filewrite"
		self.validExts = set()
		'''The valid extensions for this option.'''
	def dumpInfo(self,toStr):
		'''
		Dump packed info on this thing.
		@param toStr: The stream to write to.
		'''
		_dumpFileIOItems(self, toStr, False)


class ArgumentOptionFolderRead(ArgumentOptionString):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a folder read option.
		@param useName: The name of this option.
		'''
		ArgumentOptionString.__init__(self,useName)
		self.extTypeCode = "folderread"
		self.required = False
		'''Whether this option is required.'''
	def idiotCheck(self):
		if self.required and (len(self.value) == 0):
			raise ValueError("Missing value for " + self.name)


class ArgumentOptionFolderWrite(ArgumentOptionString):
	'''Read a file.'''
	def __init__(self, useName):
		'''
		Set up a folder write option.
		@param useName: The name of this option.
		'''
		ArgumentOptionString.__init__(self,useName)
		self.extTypeCode = "folderwrite"
		self.required = False
		'''Whether this option is required.'''
	def idiotCheck(self):
		if self.required and (len(self.value) == 0):
			raise ValueError("Missing value for " + self.name)


class StandardProgram:
	'''A program.'''
	def __init__(self):
		'''
		Set up basic stuff for a program.
		'''
		self.name = ""
		'''The name of this program.'''
		self.summary = ""
		'''A one line summary of this program.'''
		self.description = ""
		'''A full description of this program: empty if the summary will do.'''
		self.usage = ""
		'''An example of the usage of this program.'''
		self.version = ""
		'''Version information on this program.'''
		self.options = [ArgumentOptionHelp(),ArgumentOptionVersion(),ArgumentOptionArgdump(),ArgumentOptionIdiot()]
		'''The options of this program.'''
		self.useIn = sys.stdin.buffer
		'''Where to get input from.'''
		self.useOut = sys.stdout.buffer
		'''Where to send output to.'''
		self.useErr = sys.stderr.buffer
		'''Where to send error messages to.'''
		self.needRun = True
		'''Whether this thing needs to run.'''
		self.needIdiot = True
		'''Whether this thing needs to idiot check.'''
		self.wasError = False
		'''Whether there were any errors.'''
	def parse(self,forArgs):
		'''
		Parse the arguments of this program.
		@param forArgs: The arguments of interest.
		'''
		try:
			curArgs = forArgs[:]
			while(len(curArgs) > 0):
				handled = False
				for subArg in self.options:
					if subArg.canParse(curArgs):
						curArgs = subArg.parse(curArgs, self)
						handled = True
						break
				if not handled:
					raise KeyError("Unknown command line argument: " + curArgs[0])
			if self.needIdiot:
				self.idiotCheckArguments()
				self.idiotCheck()
		except Exception as e:
			self.wasError = True
			self.needRun = False
			self.useErr.write(bytes(traceback.format_exc(e)+"\n","utf-8"))
		return
	def idiotCheck(self):
		'''
		Any additional idiot checks before running.
		'''
		return
	def idiotCheckArguments(self):
		'''
		Let the individual arguments idiot check.
		'''
		for subArg in self.options:
			subArg.idiotCheck()
	def run(self):
		'''
		Run the program.
		'''
		try:
			if self.needRun:
				self.baseRun()
		except Exception as e:
			self.wasError = True
			self.useErr.write(bytes(str(e)+"\n","utf-8"))
	def baseRun(self):
		'''
		Actually do the thing.
		'''
		raise NotImplementedError("Use a subclass.")
	def printHelp(self, toStr):
		'''
		Print help information on this thing.
		@param toStr: The place to write.
		'''
		toStr.write(bytes(self.name + "\n","utf-8"))
		toStr.write(bytes(self.summary + "\n","utf-8"))
		for subArg in self.options:
			if not subArg.isCommon:
				continue
			toStr.write(bytes("  " + subArg.name + " : " + subArg.summary + "\n", "utf-8"))
			if len(subArg.usage) > 0:
				toStr.write(bytes("    " + subArg.usage + "\n", "utf-8"))
	def printVersion(self, toStr):
		'''
		Print out version information on this thing.
		@param toStr: The place to write.
		'''
		toStr.write(bytes(self.version, "utf-8"))
	def dumpArguments(self, toStr):
		'''
		Dump out information on this program.
		@param toStr: The place to write.
		'''
		# name
		curD = bytes(self.name,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# summary
		curD = bytes(self.summary,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# description
		curD = bytes(self.description,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# usage
		curD = bytes(self.usage,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# arguments
		toStr.write(struct.pack(">Q",len(self.options)))
		for subArg in self.options:
			subArg.dumpInfo(toStr)


class StandardProgramSet:
	'''A collection of programs.'''
	def __init__(self):
		'''
		Set up basic stuff for a program set.
		'''
		self.name = ""
		'''The name of this program.'''
		self.summary = ""
		'''A one line summary of this program.'''
		self.version = ""
		'''Version information on this program.'''
		self.programs = {}
		'''Constructors of each program.'''
	def parseArguments(self, forArgs, useIn, useOut, useErr):
		'''
		Parse arguments and figure out what to run.
		@param forArgs: The arguments to parse.
		@param useIn: The place to read input from.
		@param useOut: The place to write output to.
		@param useErr: The place to log errors.
		@return: The program to run, or None if nothing should be run.
		'''
		# figure out the program to run
		retProg = None
		try:
			if len(forArgs) == 0:
				raise IndexError("No program provided.")
			progN = forArgs[0]
			if (progN == "--help") or (progN == "-h") or (progN == "/?"):
				self.printHelp(useOut)
				return None
			if (progN == "--version"):
				self.printVersion(useOut)
				return None
			if (progN == "--help_argdump"):
				self.dumpPrograms(useOut)
				return None
			if not (progN in self.programs):
				raise KeyError("Program not found: " + progN)
			retProg = self.programs[progN]()
		except Exception as e:
			useErr.write(bytes(traceback.format_exc(e)+"\n","utf-8"))
			raise e
		# let that program parse
		retProg.useIn = useIn
		retProg.useOut = useOut
		retProg.useErr = useErr
		retProg.parseArguments(forArgs[1:])
		if retProg.wasError:
			raise ValueError("Problem parsing arguments.")
		return retProg
	def printHelp(self, toStr):
		'''
		Print help information on this thing.
		@param toStr: The place to write.
		'''
		toStr.write(bytes(self.name + "\n","utf-8"))
		toStr.write(bytes(self.summary + "\n","utf-8"))
		toStr.write(bytes("\n","utf-8"))
		for subPName in self.programs:
			curProg = self.programs[subPName]()
			toStr.write(bytes(subPName + "\n"))
			toStr.write(bytes(curProg.summary + "\n"))
	def printVersion(self, toStr):
		'''
		Print out version information on this thing.
		@param toStr: The place to write.
		'''
		toStr.write(bytes(self.version, "utf-8"))
	def dumpPrograms(self, toStr):
		'''
		Print out version information on this thing.
		@param toStr: The place to write.
		'''
		# name
		curD = bytes(self.name,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# summary
		curD = bytes(self.summary,"utf-8")
		toStr.write(struct.pack(">Q",len(curD)))
		toStr.write(curD)
		# number of programs
		toStr.write(struct.pack(">Q",len(self.programs)))
		# programs
		for subPName in self.programs:
			curProg = self.programs[subPName]()
			# name
			curD = bytes(subPName,"utf-8")
			toStr.write(struct.pack(">Q",len(curD)))
			toStr.write(curD)
			# summary
			curD = bytes(curProg.summary,"utf-8")
			toStr.write(struct.pack(">Q",len(curD)))
			toStr.write(curD)






