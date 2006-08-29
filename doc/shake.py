# This code is still very expiremental :
# it' my first Python code and I never tested it.
# http://rgruet.free.fr/PQR24/PQR2.4.html
# http://seriot.ch/Python
import os
import string

class frag:
    """Represent a fragment of a file"""
    def __init__(self, pos, size):
	self.pos = int(pos)
	self.size = int(size)

class file:
    """Group informations about a shaked file"""
    def __init__(self, ideal, start, end, fragc,
		 crumbc, age, guilty, name, frags): 
	self.ideal = int(ideal)
	self.start = int(start)
	self.end = int(end)
	self.fragc = int(fragc)
	self.crumbc = int(crumbc)
	self.age = int(age)
	self.guilty = int(guilty)
	self.name = string(name)
	self.frags = frags

class shake:
    """Represent a shake instance"""
    def __init__(self, name, opts="-pvv", program="shake"): 
        self.opts = opts
	self.name = name
	self.program = program
	self.files = []
	self.errors = []
    def shake(self):
	"""Run shake"""
	output = os.popen(self.program+" "+self.opts+" -- "+self.name, "r", 4096)
	title = output.readline().split('\t')
	haveFrags =  len(title) == 9
	for rawline in output:
	    print "line :"+rawline
	    line = rawline[:-1].split('\t')
	    if rawline.startswith(self.name):
		print "error :"+rawline
		self.errors.append(rawline)
		cycle
	    fraglist = []
	    if haveFrags:
		for rawfrag in line[8].split(',') :
		    rawfrag=rawfrag.split(':')
		    fraglist.append(frag(pos = rawfrag[0], size = rawfrag[1]))
	    self.files.append(file(ideal = line[0], start = line[1],
				   end = line[2], fragc = line[3],
				   crumbc = line[4], age = line[5],
				   guilty = line[6], name = line[7],
				   frags = fraglist))
    def show(self):
	"""Write debug information"""
	for file in self.files:
	    print "====="+file.name
	    for frag in file.frags:
		print frag.__dict__
	    print file.__dict__

s = shake("doc", "-pvvv", "./shake")
s.shake()
s.show()
