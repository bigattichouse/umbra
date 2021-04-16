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

# functional style pattern matching
in functional languages (Erlang style below), processing a list is accomplished by pattern matching. In psuedocode it might look like this:

```
assume we have a list of records so:

[
{resident, "Florida", "Patty"},
{resident, "Illinois", "Mike"},
{resident, "Illinois", "Joe"},
{resident, "Georgia", "Erin"}
]

findIllinoisResidents(List) -> findIllinoisResidents(List,[]).

findIllinoisResidents([{resident, "Illinois", Name}|TheRestOfTheList],Results) ->
           findIllinoisResidents(TheRestOfTheList,[{resident, "Illinois", Name}|Results]);
findIllinoisResidents([_DontCareAboutThis|TheRestOfTheList],Results) ->
           findIllinoisResidents(TheRestOfTheList,Results);
findIllinoisResidents([],Results)-> Results.

```
We have a series of functions that look the same, each saying "If THE FIRST ITEM" of a list looks like X, then I want to process that item. I then recursively call my functions again with the rest of that list. The pattern matching then goes down until it finds a pattern that fits the next item.  Above, I have four scenarios:  
1. The function the user is likely to call with a generic list. It then preps a place for results, and calls the actual working functions.
2. A list where theres an atom (like a keyword) for resident and a matching string for "Illinois", we want to append this to the Results, and keep looping
3. A list where the first item wasn't handled by #2 above, just keep looking
4. The function is called with an empty list

Each step, the list gets shorter, and the results gets longer, until the results are handed back.  The REALLY cool thing, is that we can keep the list order the same, and break this up over multiple processes, running many many threads (even on remote machines), as long as they are re-assembled in the same order.

If we design our kernel this way, asking the data blocks ONLY for the relevant fields we care about, we can skip a lot of this syntactic sugar... we just assume, by default that any case not explicitly in the matching query is a "don't care"/ignore.  There are other cases here not explained, but the general idea is that we want to be able to define what a record should look like (except in JSON or as a struct), and get records back that fit that category based on our 0.0-1.0 matching.

In the case of JSON
```
{resident:"Illinois"}
```
or C-structs

```
struct Customer {
    int id;
    char* name;
    char* resident;
};

{1,"John Q Public","Illinois"}
```

So, we have three stages:  
1. Simple pattern matching handled before the kernel is executed.
2. Determining a score/applicability (kernel)
3. Returning Modified results (kernel - not addressed yet)


# URIs can provide us user information

We plan on every record having a UUID (even copies of the record).. some big ol' hash (SHA512?) random beast.  Great. Now we need to find that record.  Perhaps we look for a patient by ID.

```
json://myserver/patients/[UUID]

```
I'm using JSON, because why not just come out and say it in the protocol field. json implies REST based http.  I could put MySQL: or SQL: or CSV: ... this is like a contract. "I want to access data at this location using this method"

but what if I want to see patients at a facility?

```
json://myserver/facilities/Downtown Clinic/patients/[UUID]

```
This, to the caller, looks like a logical path - but to the server I know what I need an index on facilities!  Perhaps we could at least log this (Warning: create index facilities )  /patients could give me a list of patients at the given facility, while /patients/UUID gives me a specific patient. Or we could rely on post/get args to filter:
```
json://myserver/facilities/Downtown Clinic/patients/?name=Michael Johnson
```









