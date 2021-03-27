# umbra
Distributed compiled webassembly database and "shaders" for lightning fast reads and queries, occasional writes

# problem space
Certain datasets, particularly time series datasets like patient data, very rarely change/amend data once it's created. There may be "diffs" of invalid data (ex: blood pressure was miskeyed at 1035 instead of 103.5, and later amended.), but for most data, it is immutable once entered. These sorts of log entries have no real reason to change. Think twitter.  You can create a tweet, it can be liked and retweeted forever, but your only option for a change is to delete it. Think things like the user's current account profile. In reality, this could be seen as a time series item as well. Your address today, changed tomorrow, is just a version of the previous record.

# compiled data + shaders
RB trees and other types of linked lists can be compiled directly into code. Code can be loaded dyanamically as needed, across multiple machines in a cluster.  Providing a small bit of "query" code like a 3D shader, we could distribute execution over a cluster of machines very easily. If that code were in a standardized format, like webassembly, then queries (shaders), execution (data), etc could even run in a browser, or as part of a ETH-style smart contract.  Concurrent reads become trivial, since write operations on much of the data are rare.

# moving mutable data
Data that *is* changed can be extracted from the code, or ignored due to a record version number, an manipulated in memory.
