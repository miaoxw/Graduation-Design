import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.security.acl.LastOwnerException;
import java.text.DecimalFormat;
import java.util.LinkedList;
import java.util.Scanner;

public class Main
{
	public static FileInputStream inputStream;
	public static Scanner scanner;
	public static FileOutputStream outputStream;
	public static OutputStreamWriter fileWriter;
	
	private static boolean rising=false;
	
	// Parameters to adjust
	public static final double PEDOMETER_THRESHOLD_LOWER_BOUND=1.2;
	public static final double PEDOMETER_THRESHOLD_UPPER_BOUND=4;
	public static final double PEDOMETER_GRAVITY_BASIS=1.0;
	public static final long FOOTSTEP_TIME_LOWER_BOUND=13;
	public static final long FOOTSTEP_TIME_UPPER_BOUND=100;
	
	public static LinkedList<Double> accelerationRecord;
	public static long lastPeakTick;
	public static long tick;
	
	public static void main(String[] args)
	{
		try
		{
			inputStream=new FileInputStream("F:/文档/大四/毕业设计/Test Input/袋中走动.txt");
			scanner=new Scanner(inputStream);
			File writtenFile=new File("F:/文档/大四/毕业设计/Test Input/算法日志.txt");
			if(writtenFile.exists())
				writtenFile.delete();
			writtenFile.createNewFile();
			outputStream=new FileOutputStream(writtenFile);
			fileWriter=new OutputStreamWriter(outputStream,"utf-8");
			fileWriter.write("原始加速度,计步\r\n");
		}
		catch(FileNotFoundException e)
		{
			System.err.println("File not found.");
			return;
		}
		catch(IOException e)
		{
			System.err.println("Create file failed.");
			return;
		}
		
		init();
		
		long stepCount=0;
		while(scanner.hasNextDouble())
		{
			double acceleration=scanner.nextDouble();
			if(judgeFootStep(acceleration))
				stepCount++;
		}
		
		System.out.println("Result: "+stepCount);
		
		try
		{
			scanner.close();
			inputStream.close();
			fileWriter.close();
			outputStream.close();
		}
		catch(IOException e)
		{
			System.err.println("IOException.");
		}
	}
	
	public static void init()
	{
		accelerationRecord=new LinkedList<>();
		tick=0;
	}
	
	public static boolean judgeFootStep(double acceleration)
	{
		tick++;
		
		if(Double.isNaN(acceleration))
			return false;
		
		boolean ret=true;
		
		accelerationRecord.add(acceleration);
		if(accelerationRecord.size()<41)
			ret=false;
		else
		{
			if(accelerationRecord.size()>41)
				accelerationRecord.removeFirst();
			
			// 寻潜在波峰，剔除休息状态
			ret=accelerationRecord.get(20)>accelerationRecord.get(19)&&accelerationRecord.get(20)>accelerationRecord.get(21)&&accelerationRecord.get(20)>=PEDOMETER_THRESHOLD_LOWER_BOUND;
			
			// 阈值判断
			if(ret)
			{
				boolean isPeak=true;
				// i==1已经在第一步判定过
				for(int i=2;i<=12;i++)
					if(accelerationRecord.get(20)<accelerationRecord.get(20-i)||accelerationRecord.get(20)<accelerationRecord.get(20+i))
					{
						isPeak=false;
						break;
					}
				if(tick-20-lastPeakTick>=FOOTSTEP_TIME_LOWER_BOUND&&tick-20-lastPeakTick<=FOOTSTEP_TIME_UPPER_BOUND)
					ret=true;
				else
					ret=false;
				ret=ret&&isPeak;
				if(isPeak)
					lastPeakTick=tick-20;
			}
			
			try
			{
				DecimalFormat format=new DecimalFormat("0.######");
				fileWriter.write(format.format(accelerationRecord.get(20))+",");
				fileWriter.write((ret?1:0)+"\r\n");
			}
			catch(IOException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return ret;
	}
}
