import argparse
import contextlib
import lzma
import tarfile
import urllib.request
import os
from concurrent.futures import ProcessPoolExecutor


def unpack(file_name_tar_xz):
    print("Starting decompression ..")
    with contextlib.closing(lzma.LZMAFile(file_name_tar_xz)) as xz:
        with tarfile.open(fileobj=xz) as f:
            f.extractall('../data')


def main():
    # This script should be in the same directory as the MATor binary. Saves results in ../data/ and ../data/consensuses
    ## Check args

    parser = argparse.ArgumentParser(
        description='Download and decompress Tor consensus files(if needed) for the given period of time. Only use reasonable input.')
    parser.add_argument('-sm', '--start_month', help='Start at month XX', required=True, type=int)
    parser.add_argument('-sy', '--start_year', help='Start in year XXXX', required=True, type=int)
    parser.add_argument('-em', '--end_month', help='End at month XX', required=True, type=int)
    parser.add_argument('-ey', '--end_year', help='End in year XXXX', required=True, type=int)
    args = parser.parse_args()

    from_year = args.start_year
    from_month = args.start_month
    to_year = args.end_year
    to_month = args.end_month

    if from_year > to_year:
        print("start_year has to be before or equal end_year")
    if not(from_month in range(1, 13)):
        print("start_month has to be between 1 and 12")
    if not(to_month in range(1, 13)):
        print("start_month has to be between 1 and 12")
    if from_year == to_year and from_month > to_month:
        print("If years are equal start_month has to be before or equal to end_month")

    if to_year - from_year == 0:
        total_downloads = to_month - from_month + 1
    else:
        total_downloads = 13 - from_month + (to_year - from_year - 1) * 12 + to_month

    years = range(from_year, to_year+1)
    months = [f"{n:02}" for n in range(1, 13)]

    current_month_index = months.index(f"{from_month:02}")
    counter = 1
    with ProcessPoolExecutor(3) as executor:
        for year in years:
            while current_month_index < 12:
                month = months[current_month_index]
                if year == to_year and (to_month - 1 < current_month_index):
                    break
                print(month, year)
                file_name_tar_xz = "../data/consensuses-" + year.__str__() + "-" + month + ".tar.xz"
                consensuses_folder = "../data/consensuses-" + year.__str__() + "-" + month
                url = "https://collector.torproject.org/archive/relay-descriptors/consensuses/consensuses-" + year.__str__() + "-" + month + ".tar.xz"
                if os.path.exists(file_name_tar_xz):
                    print("File exists :" + file_name_tar_xz)
                if os.path.exists(consensuses_folder):
                    print("File exists :" + consensuses_folder)

                # Downloading consensus data
                if (not os.path.exists(file_name_tar_xz)) and (not os.path.exists(consensuses_folder)):
                    print("Starting download " + counter.__str__() + "/" + total_downloads.__str__())
                    urllib.request.urlretrieve(url, file_name_tar_xz)
                    print(".. finished")
                counter += 1
                # Decompressing file
                if not os.path.exists(consensuses_folder):
                    executor.submit(unpack, file_name_tar_xz)
                    print("next")
                current_month_index += 1
            current_month_index = 0


if __name__ == '__main__':
    main()
