#! /usr/bin/env python
import sys
from concurrent.futures import ProcessPoolExecutor
from stem.descriptor.reader import DescriptorReader
import sqlite3 as lite
import os.path
import os
import urllib.request
import argparse


def my_reporthook(chunk_number, max_size_chunk, total_size):
    if chunk_number == 0:
        print("Total download size: " + str(total_size))
        print("Progress bar")
        print("|**************************************************|")
        sys.stdout.write("|")
        sys.stdout.flush()
    elif chunk_number % round((total_size / max_size_chunk) / 50) == 0:
        sys.stdout.write("#")
        sys.stdout.flush()
    if chunk_number * max_size_chunk >= total_size:
        sys.stdout.write("|")
        sys.stdout.flush()
        print(" ")


def unpacking_and_database_creation(database_name, file_name, database_csv_name):
    debug_level = 2
    con = None
    table_name = 'server_descriptors'
    print("Processing file: " + file_name)

    # Creating the sqlite3 database and the view
    if not os.path.exists(database_name):
        if debug_level >= 2:
            print("Creating the database and connecting to it ..")
        con = lite.connect(database_name)
        sql_query = '\
    	CREATE TABLE ' + table_name + '\
    	(\
    	address 			TEXT,\
    	family 				TEXT,\
    	fingerprint 		TEXT,\
    	nickname 			TEXT,\
    	published 			TEXT\
    	);'
        if debug_level >= 3:
            print(".. with the following sql query: ")
            print(sql_query)
        try:
            con.execute(sql_query)
        except lite.Error as e:
            print("Error %s:" % e.args[0])
            sys.exit(1)
        if debug_level >= 2:
            print(".. finished.")
        # Create the view familyview
        if debug_level >= 2:
            print("Creating the view 'familyview' ..")
        sql_query_familyview = "CREATE VIEW familyview AS SELECT nickname || '@' || address || ' ' || published AS hash, * FROM server_descriptors WHERE \
    								 family != 'set([])' AND hash != 'nickname@address published'"
        if debug_level >= 3:
            print(".. with the following sql query .. ")
            print(sql_query_familyview)
        try:
            con.execute(sql_query_familyview)
        except lite.Error as e:
            print("Error %s:" % e.args[0])
            sys.exit(1)
        if debug_level >= 2:
            print(".. finished.")

    else:
        if debug_level >= 2:
            print("Connecting to the database ..")
        con = lite.connect(database_name)
        if debug_level >= 2:
            print(".. finished.")

    if os.path.exists(file_name) and not os.path.exists(database_csv_name):
        with DescriptorReader(file_name) as reader:
            with open(database_csv_name, 'a') as csv_file:
                csv_file.write("address,family,fingerprint,nickname,published\n")
                values_list = []
                insert_string = 'INSERT INTO ' + table_name + ' VALUES (?,?,?,?,?)'
                # i = 0
                # last_i = 0
                if debug_level >= 2:
                    print("Reading the server descriptors ..")
                for desc in reader:
                    con.execute(insert_string, (
                        str(desc.address), str(desc.family), str(desc.fingerprint), str(desc.nickname),
                        str(desc.published)))

    # finally:
    if con:
        if debug_level >= 2:
            print("Closing the database ..")
        con.commit()
        con.close()
        if debug_level >= 2:
            print(".. finished.")
    if os.path.exists(database_csv_name):
        os.remove(database_csv_name)
    # if os.path.exists(file_name):
    #     os.remove(file_name)


def main():
    # This script should be in the same directory as the MATor binary. Saves results in ../data/
    parser = argparse.ArgumentParser(
        description='Download server descriptors(if needed) and construct databases for the given period of time. Only use reasonable input.')
    parser.add_argument('-sm', '--start_month', help='Start at month XX', required=True, type=int)
    parser.add_argument('-sy', '--start_year', help='Start in year XXXX', required=True, type=int)
    parser.add_argument('-em', '--end_month', help='End at month XX', required=True, type=int)
    parser.add_argument('-ey', '--end_year', help='End in year XXXX', required=True, type=int)
    args = parser.parse_args()
    debug_level = 2  # With debug level of 0 no debug information is printed

    database_name_prefix = '../data/server_descriptors'

    from_year = args.start_year
    from_month = args.start_month
    to_year = args.end_year
    to_month = args.end_month

    if from_year > to_year:
        print("start_year has to be before or equal end_year")
    if not (from_month in range(1, 13)):
        print("start_month has to be between 1 and 12")
    if not (to_month in range(1, 13)):
        print("start_month has to be between 1 and 12")
    if from_year == to_year and from_month > to_month:
        print("If years are equal start_month has to be before or equal to end_month")

    years = range(from_year, to_year + 1)
    months = [f"{n:02}" for n in range(1, 13)]
    current_month_index = months.index(f"{from_month:02}")

    with ProcessPoolExecutor(3) as executor:
        for year in years:
            while current_month_index < 12:
                month = months[current_month_index]
                if year == to_year and to_month - 1 < current_month_index:
                    break
                current_month_index += 1
                print(month, year)
                year_month = year.__str__() + "-" + month
                url = "https://collector.torproject.org/archive/relay-descriptors/server-descriptors/server-descriptors-" + year_month + ".tar.xz"
                file_name = "../data/server-descriptors-" + year_month + ".tar.xz"
                database_name = database_name_prefix + '-' + year_month + '.sqlite'
                database_csv_name = database_name_prefix + '-' + year_month + '_tmp.csv'

                if os.path.exists(file_name):
                    print("File exists :" + file_name)
                if os.path.exists(database_name):
                    print("File exists :" + database_name)

                # Downloading file
                if (not os.path.exists(file_name)) and (not os.path.exists(database_name)):
                    if debug_level >= 2:
                        print("Starting download .. " + year_month)
                    urllib.request.urlretrieve(url, file_name, my_reporthook)
                    if debug_level >= 2:
                        print(".. finished")
                if not os.path.exists(database_name):
                    executor.submit(unpacking_and_database_creation, database_name, file_name,
                                    database_csv_name)
                print("next")
            current_month_index = 0


if __name__ == '__main__':
    main()
