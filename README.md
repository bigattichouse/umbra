# umbra
Distributed compiled webassembly database and "shaders" for lightning fast reads and queries, occasional writes

# problem space
Certain datasets, particularly time series datasets like patient data, very rarely change/amend data once it's created. There may be "diffs" of invalid data (ex: blood pressure was miskeyed at 1035 instead of 103.5, and later amended.), but for most data, it is immutable once entered. These sorts of log entries have no real reason to change. Think twitter.  You can create a tweet, it can be liked and retweeted forever, but your only option for a change is to delete it. Think things like the user's current account profile. In reality, this could be seen as a time series item as well. Your address today, changed tomorrow, is just a version of the previous record.
