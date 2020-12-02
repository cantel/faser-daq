#!/usr/bin/env python3

import cx_Oracle
import json
import logging

logger = logging.getLogger('dbaccess')
db_pool = None


def init(config):
    global db_pool
    db_pool=cx_Oracle.SessionPool(config["ora_acc"],config["ora_pass"],config["ora_name"], 2, 4, 1, threaded=True)

def initDB():
    conn = db_pool.acquire()
    cursor = conn.cursor()
#    cursor.execute("DROP TABLE faser_ora_dev.RunInfoTable")
    cursor.execute(
    """CREATE TABLE faser_ora_dev.RunInfoTable
  ( 
    "RUNNUMBER" NUMBER NOT NULL PRIMARY KEY , 
    "TYPE" VARCHAR2(200 BYTE) NOT NULL , 
    "VERSION" VARCHAR2(200 BYTE) NOT NULL , 
    "CONFIGNAME" VARCHAR2(200 BYTE) NOT NULL , 
    "START_TIME" TIMESTAMP NOT NULL , 
    "STOP_TIME" TIMESTAMP NOT NULL , 
    "CONFIGURATION" CLOB CHECK (CONFIGURATION is json), 
    "RUNINFO" CLOB CHECK (RUNINFO is json)
  )
    """)


def insertNewRun(data,first=False):
    conn = db_pool.acquire()
    cursor = conn.cursor()
    if type(data["configuration"])!=str:
        data["configuration"]=json.dumps(data["configuration"])
    if first:
        insertQuery = """insert into RunInfoTable (RUNNUMBER, TYPE, VERSION, CONFIGNAME, START_TIME, STOP_TIME, CONFIGURATION) 
                         values (1, :type, :version, :configName, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :configuration) 
                      """
    else:
        insertQuery = """insert into RunInfoTable (RUNNUMBER, TYPE, VERSION, CONFIGNAME, START_TIME, STOP_TIME, CONFIGURATION) 
                         select RUNNUMBER+1, :type, :version, :configName, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :configuration from RunInfoTable
                         where rownum=1 order by RUNNUMBER DESC"""
    try:
        cursor.execute(insertQuery,data)
        cursor.execute("select RUNNUMBER from RunInfoTable where rownum=1 order by RUNNUMBER DESC")
        conn.commit()
        res=cursor.fetchone()
        if not res:
            logger.error("Did not get runnumber")
            return 0
        return res[0]

    except cx_Oracle.Error as e:
        logger.error("Failed to insert new run: "+str(e))
        return 0

def addRunInfo(runno,runinfo):
    conn = db_pool.acquire()
    cursor = conn.cursor()

    if type(runinfo)!=str: runinfo=json.dumps(runinfo)
    
    timeQuery = """select START_TIME, STOP_TIME from RunInfoTable where RUNNUMBER=:runno"""
    cursor.execute(timeQuery,runno=runno)
    start,stop=cursor.fetchone()
    if start==stop:
        updateQuery = """update RunInfoTable set STOP_TIME=CURRENT_TIMESTAMP, RUNINFO=:runinfo where RUNNUMBER=:runno"""
    else:
        updateQuery = """update RunInfoTable set RUNINFO=:runinfo where RUNNUMBER=:runno"""
    try:
        cursor.execute(updateQuery,runno=runno,runinfo=runinfo)
        conn.commit()
    except cx_Oracle.Error as e:
        logger.error(f"Failed to update RunInfo for run {runno}: "+str(e))
        return False
    return True

def getRunInfo(runno):
    conn = db_pool.acquire()
    cursor = conn.cursor()
    infoQuery = """select json_object (
                   'runnumber'  value RUNNUMBER,
                   'type'       value TYPE,
                   'version'    value VERSION,
                   'configName' value CONFIGNAME,
                   'starttime'  value START_TIME,
                   'stoptime'   value STOP_TIME
                   ), configuration,RunInfo  from RunInfoTable where RUNNUMBER=:runno"""
    try:
        cursor.execute(infoQuery,runno=runno)
        result=cursor.fetchone()
        if not result:
            logger.error("Did not get run information")
            return None
        res,configuration,runinfo=result #annoyingly json_object doesn't support returning CLOBs
        try:
            data=json.loads(res)
            if runinfo:
                data["runinfo"]=json.loads(str(runinfo))
            else:
                data["runinfo"]=None
            data["configuration"]=json.loads(str(configuration))
        except json.decoder.JSONDecodeError:
            logger.error("Failed to decode json content")
            return None
        return data
    except cx_Oracle.Error as e:
        logger.error("Failed to find run: "+str(e))
        return None


def getRunList(query):
    conn = db_pool.acquire()
    cursor = conn.cursor()

    #TODO: add query requirements

    listQuery = """select json_object (
                   'runnumber'  value RUNNUMBER,
                   'type'       value TYPE,
                   'version'    value VERSION,
                   'configName' value CONFIGNAME,
                   'starttime'  value START_TIME,
                   'stoptime'   value STOP_TIME
                   ) from RunInfoTable order by runnumber desc"""
    try:
        cursor.execute(listQuery)
        runs=[]
        for row in cursor:
            runs.append(json.loads(row[0]))
        return runs

    except cx_Oracle.Error as e:
        logger.error("Failed to get run list: "+str(e))
        return None
