{-# LANGUAGE OverloadedStrings #-}

-- On https://hackage.haskell.org/package/attoparsec-0.14.4/docs/Data-Attoparsec-ByteString.html#v:takeWhile, it's mentioned
-- Note: Because this parser does not fail, do not use it with combinators such as many, because such parsers loop until a failure occurs.
-- Careless use will thus result in an infinite loop.
--
-- This proves an infinite loop happens, invoke it like:
--
-- printf " " | ./build/attoparsec_infinite.o

module Main where

import Control.Applicative (many)
import Data.Attoparsec.Text (Parser, parseOnly)
import qualified Data.Attoparsec.Text as A
import qualified Data.Text as T
import qualified Data.Text.IO as TIO

infManySpaces :: Parser [T.Text]
infManySpaces = many (A.takeWhile (== ' '))

main :: IO ()
main = do
  input <- TIO.getContents
  putStrLn "Will not terminate"
  print (parseOnly infManySpaces input)
