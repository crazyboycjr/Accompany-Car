/* This program is for test and learning */

import org.apache.spark.SparkContext
import org.apache.spark.SparkConf

import scala.collection.mutable.ArrayBuffer

object CompCar {
  val timeInterval = 3 * 60

  def main(args: Array[String]): Unit = {
    val conf = new SparkConf().setAppName("Accompany Car")
    val sc = new SparkContext(conf)
    if (args.size != 1)
      printf("Usage: app [URI]\n")
    val inputFile = args(0)
    val textFile = sc.textFile(inputFile, 200).cache()
    val sp: Long = 1420056000 - 86400
    val res = textFile.map(line => {
      val Array(car_s, xr_s, ts_s) = line.split(",")
      val (car, xr, ts) = (car_s.toInt, xr_s.toInt, ts_s.toLong)
      //val part = if (ts < sp) 0 else 1 + (ts - sp) / 86400
      val part = ((ts - sp) / 86400).toInt
      val arr = new ArrayBuffer[(Int, Long)](1)
      arr.append((car, ts))
      ((part, xr), arr)
    }).reduceByKey(_ ++ _, numPartitions = 200)

    res.flatMap(tup => {
      val ((part, xr), arr) = tup
      val offset = (part * 86400).toLong + sp
      val carList = new Array[ArrayBuffer[Int]](86400)
      for (i <- 0 until 86400)
        carList(i) = new ArrayBuffer[Int]
      arr.foreach(v => {
        val (car, ts) = v
        carList((ts - offset).toInt).append(car)
      })
      val ret = new ArrayBuffer[(Int, Int, Int)]
      arr.foreach(v => {
        val (car, ts) = v
        var start = (ts - offset).toInt
        for (i <- start until start + timeInterval if i < 86400)
          for (cary <- carList(i) if cary != car)
            if (car < cary)
              ret.append((part, car, cary))
            else
              ret.append((part, cary, car))
      })
      ret
    }).map(word => (word, 1))
      .reduceByKey(_ + _, numPartitions = 200)
      .filter(v => v._2 >= 50)
      .sortBy(v => (-v._2, v._1._1, v._1._2, v._1._3), numPartitions = 200)
      .collect()
      .foreach(println)
  }
}
