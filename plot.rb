#!/usr/bin/ruby

require "nyaplot"
require "nyaplot3d"
require "csv"

if ARGV.length != 2 && ARGV.length != 3
  puts "Insert two or three dimensions as ruby plot.rb d1 d2"
  exit
end

injury_color = "#ff0000"
healthy_color = "#0000ff"

descritors = ["Homogeneidade", "Entropia", "Energia", "Contraste"]

if ARGV.length == 2
  plot = Nyaplot::Plot.new

  injuries_x = []
  injuries_y = []
  healthies_x = []
  healthies_y = []

  CSV.foreach("training.csv") do |row|
    x = row[ARGV[0].to_i].to_f
    y = row[ARGV[1].to_i].to_f

    if row[0] == "1"
      injuries_x << x
      injuries_y << y
    else
      healthies_x << x
      healthies_y << y
    end
  end

  sc = plot.add(:scatter, injuries_x, injuries_y)
  sc.color(injury_color)
  sc.title("Lesoes")

  sc = plot.add(:scatter, healthies_x, healthies_y)
  sc.color(healthy_color)
  sc.title("Regioes Sadias")

  plot.x_label(descritors[ARGV[0].to_i-1])
  plot.y_label(descritors[ARGV[1].to_i-1])

  plot.legend(true)

else
  plot = Nyaplot::Plot3D.new

  plot.configure do
    width(1920)
    height(1080)
  end

  injuries_x = []
  injuries_y = []
  injuries_z = []
  healthies_x = []
  healthies_y = []
  healthies_z = []

  CSV.foreach("training.csv") do |row|
    x = row[ARGV[0].to_i].to_f
    y = row[ARGV[1].to_i].to_f
    z = row[ARGV[2].to_i].to_f

    if row[0] == "1"
      injuries_x << x
      injuries_y << y
      injuries_z << z
    else
      healthies_x << x
      healthies_y << y
      healthies_z << z
    end
  end

  sc = plot.add(:scatter, injuries_x, injuries_y, injuries_z)
  sc.fill_color(injury_color)
  sc.name("Lesoes")

  sc = plot.add(:scatter, healthies_x, healthies_y, healthies_z)
  sc.fill_color(healthy_color)
  sc.name("Reg. Sadias")
end

plot.export_html "chart.html"
