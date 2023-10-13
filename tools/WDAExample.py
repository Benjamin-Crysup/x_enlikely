import sys

import whodunargs

class TestProgram(whodunargs.StandardProgram):
	def __init__(self):
		whodunargs.StandardProgram.__init__(self)
		self.name = "WDAExample"
		self.summary = "A simple example for whodunargs."
		self.usage = "python3 WDAExample"
		self.version = "WDAExample 0.0\nCopyright (C) 2023 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n"
		# a flag option
		self.flagOpt = whodunargs.ArgumentOptionFlag("--bye")
		self.flagOpt.summary = "Test flag."
		self.options.append(self.flagOpt)
		# an enum option
		self.enumVal = whodunargs.ArgumentOptionEnumValue("radio")
		self.enumOptA = whodunargs.ArgumentOptionEnum("--doa", self.enumVal)
		self.enumOptA.summary = "The first enum value."
		self.options.append(self.enumOptA)
		self.enumOptB = whodunargs.ArgumentOptionEnum("--dob", self.enumVal)
		self.enumOptB.summary = "The second enum value."
		self.options.append(self.enumOptB)
		self.enumOptC = whodunargs.ArgumentOptionEnum("--doc", self.enumVal)
		self.enumOptC.summary = "The third enum value."
		self.options.append(self.enumOptC)
		# an integer option
		self.intOpt = whodunargs.ArgumentOptionInteger("--qual")
		self.intOpt.summary = "A simple integer option."
		self.intOpt.usage = "--qual 20"
		self.intOpt.value = 20
		self.options.append(self.intOpt)
		# a float option
		self.fltOpt = whodunargs.ArgumentOptionFloat("--pow")
		self.fltOpt.summary = "A simple float option."
		self.fltOpt.usage = "--pow 1.0"
		self.fltOpt.value = 1.0
		self.options.append(self.fltOpt)
		# a string option
		self.strOpt = whodunargs.ArgumentOptionString("--name")
		self.strOpt.summary = "A simple string option."
		self.strOpt.usage = "--name abc"
		self.strOpt.value = "abc"
		self.options.append(self.strOpt)
		# an integer vector option
		self.intvOpt = whodunargs.ArgumentOptionIntegerVector("--dims")
		self.intvOpt.summary = "A simple integer vector option."
		self.intvOpt.usage = "--dims 10"
		self.options.append(self.intvOpt)
		# a float vector option
		self.fltvOpt = whodunargs.ArgumentOptionFloatVector("--cols")
		self.fltvOpt.summary = "A simple float vector option."
		self.fltvOpt.usage = "--cols 0.4"
		self.options.append(self.fltvOpt)
		# a string vector option
		self.strvOpt = whodunargs.ArgumentOptionStringVector("--tags")
		self.strvOpt.summary = "A simple string vector option."
		self.strvOpt.usage = "--tags bad"
		self.options.append(self.strvOpt)
		# a file read option
		self.filerOpt = whodunargs.ArgumentOptionFileRead("--in")
		self.filerOpt.summary = "A file read option."
		self.filerOpt.usage = "--in /home/ben/in.tsv"
		self.filerOpt.validExts.add(".tsv")
		self.options.append(self.filerOpt)
		# a file write option
		self.filewOpt = whodunargs.ArgumentOptionFileRead("--out")
		self.filewOpt.summary = "A file write option."
		self.filewOpt.usage = "--out /home/ben/in.csv"
		self.filewOpt.validExts.add(".csv")
		self.options.append(self.filewOpt)
		# a folder read option
		self.foldrOpt = whodunargs.ArgumentOptionFolderRead("--builddir")
		self.foldrOpt.summary = "A folder read option."
		self.foldrOpt.usage = "--builddir /home/ben/"
		self.options.append(self.foldrOpt)
		# a folder write option
		self.foldwOpt = whodunargs.ArgumentOptionFolderWrite("--workd")
		self.foldwOpt.summary = "A folder write option."
		self.foldwOpt.usage = "--workd /home/ben/"
		self.options.append(self.foldwOpt)
		# many file reads
		self.filervOpt = whodunargs.ArgumentOptionFileReadVector("--refs")
		self.filervOpt.summary = "A file read vector option."
		self.filervOpt.usage = "--refs /home/ben/in.bmp"
		self.options.append(self.filervOpt)
		# many file writes
		self.filewvOpt = whodunargs.ArgumentOptionFileWriteVector("--logs")
		self.filewvOpt.summary = "A file write vector option."
		self.filewvOpt.usage = "--logs /home/ben/in.midi"
		self.options.append(self.filewvOpt)
	def baseRun(self):
		self.useOut.write(bytes("Hello\n","utf-8"))
		if self.flagOpt.value:
			self.useOut.write(bytes("Goodbye\n","utf-8"))


toRun = TestProgram()
toRun.parse(sys.argv[1:])
toRun.run()

