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

def readDump(startDate, dumpFile):
	date = datetime.strptime(startDate, dateFormat).timetuple()
	timestamp = time.mktime(date)

	#os.system("clear")
	print("Start date: {}".format(startDate))

	with open(dumpFile, "r") as handle:
		lines = handle.readlines()
		index = 0
		for line in lines[2:]:
			if line[0] is not '[':
				continue
			entry = line.rstrip()
			value = entry[(entry.index(" = ") + 3):]
			entryTimestamp = int(value)
			if entryTimestamp == 0:
				break  
			
			entryDate = datetime.fromtimestamp(timestamp + entryTimestamp)
			entryDateString = entryDate.strftime(dateFormat)
			index = index + 1
			delta = abs(datetime.fromtimestamp(timestamp) - entryDate)

			seconds = delta.days * 24 * 3600 + delta.seconds
			minutes, seconds = divmod(seconds, 60)
			hours, minutes = divmod(minutes, 60)
			days, hours = divmod(hours, 24)

			diff = "{}d {}h {}m {}s".format(days, hours, minutes, seconds)
			print("Event no #{0} date: {1} ({2})".format(index, entryDateString, diff))

if __name__ == "__main__":
	print("Enter start date (format: yyyy-MM-dd HH:mm:ss): ")
	startDate = "2020-11-28 15:01:00"
	print("\nEnter dump file name: ")
	dumpFile = "data_real.dump"
	readDump(startDate, dumpFile)