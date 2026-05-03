{-# Language OverloadedStrings #-}

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

module Text.ParseCSV
   ( CSV
   , parseCSV
   ) where

import Prelude hiding (concat, takeWhile)
import Control.Applicative ((<$>), (<|>), (<*>), (<*), (*>), many)
import Control.Monad (void)
import Data.Attoparsec.Text
import qualified Data.Text as T (Text, concat, cons, append)

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
