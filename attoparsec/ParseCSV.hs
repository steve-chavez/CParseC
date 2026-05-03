{-# LANGUAGE OverloadedStrings #-}

{-
 -This is taken from https://raw.githubusercontent.com/robinbb/attoparsec-csv/refs/heads/master/Text/ParseCSV.hs,
 -modified to obtain the input from stdin instead of a file
 -}

-- Copyright 2012 UserEvents, Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--    http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
-- Contributors:
--    Robin Bate Boerop <me@robinbb.com>

module Main where

import Prelude hiding (concat, takeWhile)
import qualified Prelude as P
import Control.Applicative ((<|>), many)
import Control.Monad (void)
import Data.Attoparsec.Text
import qualified Data.Text as T (Text, concat, cons, append)
import qualified Data.Text.IO as TIO
import System.Exit (exitFailure, exitSuccess)
import System.IO (hPutStrLn, stderr)

type CSV = [[T.Text]]

lineEnd :: Parser ()
lineEnd =
   void (char '\n') <|> void (string "\r\n") <|> void (char '\r')
   <?> "end of line"

unquotedField :: Parser T.Text
unquotedField =
   takeWhile (\c -> c /= ',' && c /= '\n' && c /= '\r' && c /= '"')
   <?> "unquoted field"

insideQuotes :: Parser T.Text
insideQuotes =
   T.append <$> takeWhile (/= '"')
            <*> (T.concat <$> many (T.cons <$> dquotes <*> insideQuotes))
   <?> "inside of double quotes"
   where
      dquotes =
         string "\"\"" >> return '"'
         <?> "paired double quotes"

quotedField :: Parser T.Text
quotedField =
   char '"' *> insideQuotes <* char '"'
   <?> "quoted field"

field :: Parser T.Text
field =
   quotedField <|> unquotedField
   <?> "field"

record :: Parser [T.Text]
record =
   field `sepBy1` char ','
   <?> "record"

file :: Parser CSV
file =
   (:) <$> record
       <*> manyTill (lineEnd *> record)
                    (endOfInput <|> lineEnd *> endOfInput)
   <?> "file"

parseCSV :: T.Text -> Either String CSV
parseCSV =
   parseOnly file

main :: IO ()
main = do
   input <- TIO.getContents
   case parseCSV input of
      Left err -> do
         hPutStrLn stderr $ "csv parser fail: " ++ err
         exitFailure
      Right [] -> do
         hPutStrLn stderr $ "csv parser fail: empty input"
         exitFailure
      -- Just print some rows and the total to see it succeeded
      Right (headers : rows) -> do
         printHeaders headers
         mapM_ (uncurry (printRow headers)) (zip (P.take 2 rows) [0 ..])
         putStrLn ("Parsed " ++ show (length rows) ++ " data rows.")
         exitSuccess

printHeaders :: [T.Text] -> IO ()
printHeaders headers = do
   putStrLn "Headers:"
   mapM_ printHeader headers
   putStrLn mempty
   where
      printHeader name = do
         putStr "  - "
         TIO.putStr name
         putStrLn mempty

printRow :: [T.Text] -> [T.Text] -> Int -> IO ()
printRow headers row rowIndex = do
   putStrLn ("Row " ++ show (rowIndex + 1))
   mapM_ printColumn (zip headers (row ++ repeat mempty)) -- repeat mempty is in case it ends with empty fields, so it shows the column is empty
   putStrLn mempty
   where
      printColumn (name, value) = do
         putStr "  "
         TIO.putStr name
         putStr ": "
         TIO.putStr value
         putStrLn mempty
