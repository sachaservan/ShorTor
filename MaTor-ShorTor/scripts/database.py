import csv
import os
import sys


class Database(object):
	"""docstring for Database"""
	def __init__(self, fname, keys, defaultcols = []):
		self.fname = fname
		self.keys = keys
		self.cols = defaultcols
		self.f = None
		self.data = {} # key => {col => value}

	def addColumn(self, col):
		if col in self.cols:
			return
		self.cols.append(col)
	def addColumns(self, cols):
		for col in cols:
			if col not in self.cols:
				self.cols.append(col)
	
	def open(self):
		# Load DS from file
		if os.path.isfile(self.fname):
			f = open(self.fname, 'rb') if sys.version_info[0] < 3 else open(self.fname,'r')
			reader = csv.reader(f, delimiter=';', quotechar='"')
			headers = next(reader)
			if headers[:len(self.keys)] != self.keys:
				raise Exception("Mismatching keys!")
			for h in headers[len(self.keys):]:
				if h not in self.cols:
					self.cols.append(h)
			for x in reader:
				key = tuple(x[:len(self.keys)])
				if not key in self.data:
					self.data[key] = {}
				for i in range(len(self.keys), len(x)):
					self.data[key][headers[i]] = x[i]
			print("Database:",len(self.data),"rows loaded!")

	def close(self):
		pass

	def __enter__(self):
		self.open()
		return self
	def __exit__(self, type, value, tb):
		self.close()

	def save(self):
		# Write DS to file
		f = open(self.fname, 'wb') if sys.version_info[0] < 3 else open(self.fname, 'w', newline="\n", encoding="utf-8")
		writer = csv.writer(f, delimiter=';', quotechar='"')
		writer.writerow(self.keys + self.cols)
		for key in sorted(self.data):
			line = list(key)
			v = self.data[key]
			for col in self.cols:
				if col in v:
					line.append(v[col])
				else:
					line.append(None)
			writer.writerow(line)

	def addRow(self, keys, values):
		key = tuple(keys)
		if not key in self.data:
			self.data[key] = {}
		for i in range(0, len(values)):
			self.data[key][self.cols[i]] = self.flatValue(values[i])

	def addData(self, keys, values):
		key = tuple(keys)
		if not key in self.data:
			self.data[key] = {}
		for col in values:
			self.data[key][col] = self.flatValue(values[col])

	def hasEntry(self, keys, fullLine = True):
		key = tuple(keys)
		if key not in self.data:
			return False
		if not fullLine:
			return True
		for c in self.cols:
			if c not in self.data[key]:
				return False
		return True

	def __contains__(self, keys):
		return self.hasEntry(keys, False)
	def __getitem__(self, keys):
		return self.data[tuple(keys)]

	def flatValue(self, value):
		if isinstance(value, tuple) and len(value) == 1:
			return value[0];
		return value;


