/* This program is for test and learning */

import org.apache.spark.SparkContext
import org.apache.spark.SparkConf

object SimpleApp {
  def main(args: Array[String]): Unit = {
    val conf = new SparkConf().setAppName("Simple Application")
    val sc = new SparkContext(conf)
    val textFile = sc.textFile("hdfs:///compcar/compcar_small.csv", 1).cache()
    val result = textFile.flatMap(_.split(',')).map((_, 1)).reduceByKey(_ + _).collect()
    result.foreach(println)
    //println(textFile.first())
    //println("count = %d".format(textFile.count()))
  }
}
