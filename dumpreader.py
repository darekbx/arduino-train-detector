'''
Train detector memory dump reader

Dump structure:
[0] = 1000 # Actual timestamp (since last memory reset)
[4] = 16   # Index of the actual event address
[8] = 128  # First event
[16] = 196  # second event 
...

'''
from datetime import datetime
import time
import os

dateFormat = "%Y-%m-%d %H:%M:%S"

def formatDelta(delta):
	seconds = delta.days * 24 * 3600 + delta.seconds
	minutes, seconds = divmod(seconds, 60)
	hours, minutes = divmod(minutes, 60)
	days, hours = divmod(hours, 24)
	return "{}d {}h {}m {}s".format(days, hours, minutes, seconds)

def readDump(startDate, dumpFile):
	date = datetime.strptime(startDate, dateFormat).timetuple()
	timestamp = time.mktime(date)

	print("Start date: {}".format(startDate))

	with open(dumpFile, "r") as handle:
		lines = handle.readlines()
		index = 0	
		
		for line in lines:
			if line[0] is not '[':
				continue

			entry = line.rstrip()
			value = entry[(entry.index(" = ") + 3):]
			entryTimestamp = int(value)

			if entryTimestamp == 0:
				break

			if index == 0:
				lastTimestamp = (datetime.fromtimestamp(entryTimestamp) - datetime.fromtimestamp(0))
				fromatted = formatDelta(lastTimestamp)
				print("Last timestamp: {}".format(fromatted))
				index = index + 1
				continue

			if index == 1:
				index = index + 1
				continue

			entryDate = datetime.fromtimestamp(timestamp + entryTimestamp)
			entryDateString = entryDate.strftime(dateFormat)
			index = index + 1
			delta = abs(datetime.fromtimestamp(timestamp) - entryDate)
			diff = formatDelta(delta)
			print("Event no #{0} date: {1} ({2})".format(index, entryDateString, diff))

if __name__ == "__main__":
	startDate = "2020-12-04 20:54:00"
	dumpFile = "2020-12-04_20:54:00_v2.dump"
	readDump(startDate, dumpFile)
