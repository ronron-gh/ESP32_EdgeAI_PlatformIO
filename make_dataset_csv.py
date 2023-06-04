import os
import re
from PIL import Image
import math
import random
import sys

	
inputFolder = ''
outputFolder = 'converted_datasets'
csvFileName1 = './train.csv'
csvFileName2 = './test.csv'
imgSize = 28
trainDataRatio = 0.8

csvLines = []

patternStr = '.+\.(JPG|jpg)'
pattern = re.compile(patternStr)



def make_dataset_csv(root_folder):

	# カテゴリ毎にフォルダ分けされた画像データのリストを作る
	for foldername, subfolders, filenames in os.walk(root_folder):
		print('The current folder is ' + foldername)

		for subfolder in subfolders:
			#print('SUBFOLDER OF ' + foldername + ': ' + subfolder)
			# 変換後の画像を格納するフォルダにカテゴリ毎のフォルダを作っておく
			os.makedirs('./' + outputFolder + '/' + subfolder)

		for filename in filenames:
			#print('FILE INSIDE ' + foldername + ': ' + filename)

			result = pattern.match(filename)
			if result:
				print('listed file : ' + foldername + '/' + filename)
				category = os.path.basename(foldername)
				csvLines.append('./' + outputFolder + '/' + category + '/' + filename + ',' + category)
				
				img = Image.open(foldername + '/' + filename)
				img_gray = img.convert("L")
				img_resized = img_gray.resize((imgSize, imgSize))
				img_resized.save('./' + outputFolder + '/' + category + '/' + filename)
				

	# データセットの順番を混ぜる
	random.shuffle(csvLines)
	
	# データセットを学習用データと検証用データに分ける
	nTotalFiles = len(csvLines)
	nTrainFiles = (int)(nTotalFiles * trainDataRatio)
	csvLinesTrain = csvLines[:nTrainFiles]
	csvLinesTest = csvLines[nTrainFiles+1:]
	
	with open(csvFileName1, mode='w') as f:
		f.write('x:image,y\n')
		f.write('\n'.join(csvLinesTrain))

	with open(csvFileName2, mode='w') as f:
		f.write('x:image,y\n')
		f.write('\n'.join(csvLinesTest))


if __name__ == '__main__':
	
	args = sys.argv
	
	if 2 <= len(args):
		inputFolder = args[1]
	
	make_dataset_csv(inputFolder)



