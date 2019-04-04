
Command line interface for the Cassandra cluster:

```
python2.7 /usr/bin/cqlsh.py
```

(Without sourcing the RD51 build environment!)

Created keyspace with:

```
CREATE KEYSPACE rd51pro WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'}  AND durable_writes = true;
```

