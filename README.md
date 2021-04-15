# umbra
Distributed compiled webassembly database and "shaders" for lightning fast reads and queries, occasional writes

# problem space
Certain datasets, particularly time series datasets like patient data, very rarely change/amend data once it's created. There may be "diffs" of invalid data (ex: blood pressure was miskeyed at 1035 instead of 103.5, and later amended.), but for most data, it is immutable once entered. These sorts of log entries have no real reason to change. Think twitter.  You can create a tweet, it can be liked and retweeted forever, but your only option for a change is to delete it. Think things like the user's current account profile. In reality, this could be seen as a time series item as well. Your address today, changed tomorrow, is just a version of the previous record.

# compiled data + shaders
RB trees and other types of linked lists can be compiled directly into code. Code can be loaded dyanamically as needed, across multiple machines in a cluster.  Providing a small bit of "query" code like a 3D shader, we could distribute execution over a cluster of machines very easily. If that code were in a standardized format, like webassembly, then queries (shaders), execution (data), etc could even run in a browser, or as part of a ETH-style smart contract.  Concurrent reads become trivial, since write operations on much of the data are rare.

# moving mutable data
Data that *is* changed can be extracted from the code, or ignored due to a record version number, an manipulated in memory.

# "packetized" datsets
When discussing a particular user of a streaming system, or a patient, there is a history unique to that record. While it might be useful to aggregate all the patients at a particular facility into one querable dataset, a single patient's data could be captured in a single file that can be moved from one server to another, or anonymized attached to a test case as a snapshot.  This test case can be used as part of the preflight checks to ensure that the system is operating properly for a given behavior expectation. This "packetized" module can be linked by a compiled index as well.  When adding/amending data to the patient record, we can extract the patient easily and recompile.  

# erlang-style light processes. 
We can think of this "packet" as a process waiting for kernel shaders.  Hey, would you process this query?  Indicies would provide a measure of efficiency to only accessing the queries we want prior to actually querying a dataset. Since these datasets are transportable packets of code, we can distribute them over a wide area to be processed.  An index is just another dataset to be queried, providing pointers (URIs) to records JSON://someserver/patients/xyz123   Each record has a UUID that we can use to locate it. I prefer a purely random id to prevent guessing, even when a hash might be more convenient.

# vectorspace and boolean
In the traditional SQL world, a query would arrive, and this record would either qualify, or not. Where Facility = 'Downtown Clinic' or Hemoglobin > 11.3 g/dL.  Simple boolean.  1 or 0, true or false. I would suggest that we use a vectorspace model, allowing dimensionality to consist of 0 to 1.. so you can have 0.5 values or other "similarity" results, in conjunction with boolean outcomes.  by adjusting a query to have a threshold of 1.0 of similarity, you become boolean.  By having  partial similarity, you can do things like clustering, thesaurus querying, and other less exact searches.  You can either use a simple vector, or even have the querying kernel shader define partial success by providing a score between 0 and 1.





