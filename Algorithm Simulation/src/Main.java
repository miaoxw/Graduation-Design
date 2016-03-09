import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.text.DecimalFormat;
import java.util.Scanner;

public class Main
{
	public static FileInputStream inputStream;
	public static Scanner scanner;
	public static FileOutputStream outputStream;
	public static OutputStreamWriter fileWriter;
	
	private static boolean rising=false;
	
	//Parameters to adjust
	public static final double PEDOMETER_THRESHOLD_LOWER_BOUND=1.08;
	public static final double PEDOMETER_THRESHOLD_UPPER_BOUND=4;
	public static final double PEDOMETER_GRAVITY_BASIS=1.0;
	public static final long FOOTSTEP_TIME_LOWER_BOUND=13;
	public static final long FOOTSTEP_TIME_UPPER_BOUND=100;
	
	public static double accelerationRecord[]=new double[150];
	public static double lastAcceleration;
	public static double threshold;
	public static double thresholdCeil;
	public static double accelerationHistory[]=new double[32];
	public static int count;
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
			fileWriter.write("原始加速度,平滑加速度,threshold,计步,threshold上阈值\r\n");
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
		threshold=PEDOMETER_THRESHOLD_LOWER_BOUND;
		count=0;
		tick=0;
		lastAcceleration=PEDOMETER_GRAVITY_BASIS;
		lastPeakTick=0;
		
		for(int i=0;i<32;i++)
			accelerationHistory[i]=PEDOMETER_GRAVITY_BASIS;
	}
	
	public static boolean judgeFootStep(double acceleration)
	{
		tick++;
		
		if(Double.isNaN(acceleration))
			return false;
		
		boolean ret=false;
		
		for(int i=0;i<31;i++)
			accelerationHistory[i+1]=accelerationHistory[i];
		accelerationHistory[0]=acceleration;
		
		double filteredAcceleration=0.0;
		for(int i=0;i<32;i++)
			filteredAcceleration+=accelerationHistory[i];
		filteredAcceleration/=32;
		
		accelerationRecord[count++]=filteredAcceleration;
		if(count==150)
		{
			count=0;
			
			double accTotal=0.0;
			int peakCount=0;
			double maxAcceleration=-1e4;
			for(int i=1;i<149;i++)
			{
//				if(accelerationRecord[i]>maxAcceleration)
//					maxAcceleration=accelerationRecord[i];
				if(accelerationRecord[i]>accelerationRecord[i-1]&&accelerationRecord[i]<accelerationRecord[i+1]&&accelerationRecord[i]>PEDOMETER_GRAVITY_BASIS)
				{
					accTotal+=accelerationRecord[i];
					peakCount++;
				}
			}
			
//			threshold=maxAcceleration*0.25+PEDOMETER_GRAVITY_BASIS*0.75;
//			if(threshold<PEDOMETER_THRESHOLD_LOWER_BOUND)
//				threshold=PEDOMETER_THRESHOLD_LOWER_BOUND;
//			updateThresholdCeil();
			threshold=accTotal/peakCount;
			if(threshold<PEDOMETER_THRESHOLD_LOWER_BOUND)
				threshold=PEDOMETER_THRESHOLD_LOWER_BOUND;
		}
		
		if(filteredAcceleration>=lastAcceleration)
			rising=true;
		else
		{
			if(lastAcceleration>=threshold&&rising)
			{
				long currentTick=tick;
				long deltaTick=currentTick-lastPeakTick;
				
				if(deltaTick>FOOTSTEP_TIME_LOWER_BOUND&&deltaTick<FOOTSTEP_TIME_UPPER_BOUND)
					ret=true;
				
				lastPeakTick=currentTick;
			}
			rising=false;
		}
		
		lastAcceleration=filteredAcceleration;
		try
		{
			DecimalFormat format=new DecimalFormat("0.######");
			fileWriter.write(format.format(acceleration)+",");
			fileWriter.write(format.format(filteredAcceleration)+",");
			fileWriter.write(format.format(threshold)+",");
			fileWriter.write((ret?1:0)+",");
			fileWriter.write(format.format(thresholdCeil)+"\r\n");
		}
		catch(IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return ret;
	}
}
