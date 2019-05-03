
Command line interface for the Cassandra cluster:

```
cqlsh
```

(Without sourcing the RD51 build environment!)

Created keyspace with:

```
CREATE KEYSPACE rd51pro WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'}  AND durable_writes = true;
```

Dump table content:

```
SELECT * FROM rd51daq.payload;
```

Drop table content:

```
DROP TABLE rd51daq.payload;
```