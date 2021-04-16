# umbra
Distributed compiled webassembly database and "shaders" for lightning fast reads and queries, occasional writes

# problem space
Certain datasets, particularly time series datasets like patient data, very rarely change/amend data once it's created. There may be "diffs" of invalid data (ex: blood pressure was miskeyed at 1035 instead of 103.5, and later amended.), but for most data, it is immutable once entered. These sorts of log entries have no real reason to change. Think twitter.  You can create a tweet, it can be liked and retweeted forever, but your only option for a change is to delete it. Think things like the user's current account profile. In reality, this could be seen as a time series item as well. Your address today, changed tomorrow, is just a version of the previous record.

# umbra
Umbra takes some queues from functional programming and 3D rendering pipelines.  

Here's a list of assertions about the final system:

1. Data is compiled. Currently in C. Ultimately, it will have multiple endpoints, mainly webassembly.  I will rely on wasm/lvm and other compilers to generate for now, until I can figure out the file format and dircetly compile data to target webassembly. This will be primarily for historical/timeseries datasets that don't change much.
2. Pages of compiled data can be loaded at runtime and queried.
3. Indices are compiled, and loaded in separate modules. Same idea.
4. Data that changes will be stored in memory, until flushed/compiled to a page. This goes for appends as well.
5. Construction/Compilation can be distributed, so firehose-like loading will be possible.
6. Tables are C structs.
7. REST/JSON will be the primary interface for the consuming service, which shouldn't see most of this.
8. Table "servers" will be separate, so no one table will crash another. 
9. Table proxies/proxy servers will allow distirbution across multiple machines/processes.
10. Record versioning should be possible (just use git since we're compiled?)
11. Umbra will use "kernel" queries like shaders in 3d programming, as well as vectorspace classification to determine suitability.
    A. Guard: A vectorspace query will determine if a group of data might even be appropriate to a query.  Each page will have a "representative" vector that can quickly weed out whether a query is even appropriate. ("I only have Illinois accounts" vs. "For all California accounts...") This will be a 0 to 1 score, so thresholds for passing can be set for more vague queries. 
    B. Indices: Once the guard allows a query to process, the main indices (based on URI) are used to locate the records affected. 
    C. Shader/List:  executable code that runs against a given record (or group of records) and returns one or more records. These could be a simple boolean (city="Chicago") or a vectorspace score (0 to 1). This is operations on the list of all records allowed by the Guard/Index. May be used to sort the results, or call functions to process the patterns.
    D. Shader/Patterns:  the shader will have a list of "registered" patterns, like a switch.  The list is called similar to pattern matching in Erlang, starting at the top of the list. Results are stored in an outbound list (which could consist of a single item, say for Sum() operations)
10. Results will return a record, in the appropriate order, along with a score. Simple boolean responses would be 1|0 default is {1,{some record}}
    
    
