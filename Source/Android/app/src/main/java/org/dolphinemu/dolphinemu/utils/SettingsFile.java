package org.dolphinemu.dolphinemu.utils;

import android.os.Environment;
import android.support.annotation.NonNull;

import org.dolphinemu.dolphinemu.model.settings.BooleanSetting;
import org.dolphinemu.dolphinemu.model.settings.FloatSetting;
import org.dolphinemu.dolphinemu.model.settings.IntSetting;
import org.dolphinemu.dolphinemu.model.settings.Setting;
import org.dolphinemu.dolphinemu.model.settings.SettingSection;
import org.dolphinemu.dolphinemu.model.settings.StringSetting;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.Set;

import rx.Observable;
import rx.Subscriber;

public final class SettingsFile
{
	public static final String FILE_NAME_DOLPHIN = "Dolphin";
	public static final String FILE_NAME_GFX = "GFX";
	public static final String FILE_NAME_GCPAD = "GCPadNew";
	public static final String FILE_NAME_WIIMOTE = "WiimoteNew";

	public static final String SECTION_CORE = "Core";

	public static final String SECTION_GFX_SETTINGS = "Settings";
	public static final String SECTION_GFX_ENHANCEMENTS = "Enhancements";
	public static final String SECTION_GFX_HACKS = "Hacks";


	public static final String KEY_CPU_CORE= "CPUCore";
	public static final String KEY_DUAL_CORE= "CPUThread";
	public static final String KEY_OVERCLOCK_ENABLE= "OverclockEnable";
	public static final String KEY_OVERCLOCK_PERCENT= "Overclock";
	public static final String KEY_VIDEO_BACKEND= "GFXBackend";

	public static final String KEY_SHOW_FPS= "ShowFPS";
	public static final String KEY_INTERNAL_RES= "EFBScale";
	public static final String KEY_FSAA= "MSAA";
	public static final String KEY_ANISOTROPY= "MaxAnisotropy";
	public static final String KEY_POST_SHADER= "PostProcessingShader";
	public static final String KEY_SCALED_EFB= "EFBScaledCopy";
	public static final String KEY_PER_PIXEL= "EnablePixelLighting";
	public static final String KEY_FORCE_FILTERING= "ForceFiltering";
	public static final String KEY_DISABLE_FOG= "DisableFog";

	public static final String KEY_STEREO_MODE= "StereoMode";
	public static final String KEY_STEREO_DEPTH= "StereoDepth";
	public static final String KEY_STEREO_CONV= "StereoConvergencePercentage";
	public static final String KEY_STEREO_SWAP= "StereoSwapEyes";

	public static final String KEY_SKIP_EFB= "EFBAccessEnable";
	public static final String KEY_IGNORE_FORMAT= "EFBEmulateFormatChanges";
	public static final String KEY_EFB_COPY= "EFBCopyEnable";
	public static final String KEY_EFB_TEXTURE= "EFBToTextureEnable";
	public static final String KEY_EFB_CACHE= "EFBCopyCacheEnable";
	public static final String KEY_TEXCACHE_ACCURACY= "SafeTextureCacheColorSamples";
	public static final String KEY_XFB= "UseXFB";
	public static final String KEY_XFB_REAL= "UseRealXFB";
	public static final String KEY_FAST_DEPTH= "FastDepthCalc";
	public static final String KEY_ASPECT_RATIO= "AspectRatio";

	private SettingsFile()
	{
	}

	public static Observable<HashMap<String, SettingSection>> readFile(final String fileName)
	{
		return Observable.create(new Observable.OnSubscribe<HashMap<String, SettingSection>>()
		{
			@Override
			public void call(Subscriber<? super HashMap<String, SettingSection>> subscriber)
			{

				HashMap<String, SettingSection> sections = new HashMap<>();

				File ini = getSettingsFile(fileName);

				BufferedReader reader = null;

				try
				{
					reader = new BufferedReader(new FileReader(ini));

					SettingSection current = null;
					for (String line; (line = reader.readLine()) != null; )
					{
						if (line.startsWith("[") && line.endsWith("]"))
						{
							current = sectionFromLine(line);
							sections.put(current.getName(), current);
						}
						else if ((current != null) && line.contains("="))
						{
							Setting setting = settingFromLine(current, line);
							current.putSetting(setting.getKey(), setting);
						}
					}
				}
				catch (FileNotFoundException e)
				{
					Log.error("[SettingsFile] File not found: " + fileName + ".ini: " + e.getMessage());
					subscriber.onError(e);
				}
				catch (IOException e)
				{
					Log.error("[SettingsFile] Error reading from: " + fileName + ".ini: " + e.getMessage());
					subscriber.onError(e);
				} finally
				{
					if (reader != null)
					{
						try
						{
							reader.close();
						}
						catch (IOException e)
						{
							Log.error("[SettingsFile] Error closing: " + fileName + ".ini: " + e.getMessage());
							subscriber.onError(e);
						}
					}
				}

				subscriber.onNext(sections);
				subscriber.onCompleted();
			}
		});
	}

	public static Observable<Boolean> saveFile(final String fileName, final HashMap<String, SettingSection> sections)
	{
		return Observable.create(new Observable.OnSubscribe<Boolean>()
		{
			@Override
			public void call(Subscriber<? super Boolean> subscriber)
			{
				File ini = getSettingsFile(fileName);

				PrintWriter writer = null;
				try
				{
					writer = new PrintWriter(ini, "UTF-8");

					Set<String> keySet = sections.keySet();

					for (String key : keySet)
					{
						SettingSection section = sections.get(key);
						writeSection(writer, section);
					}
				}
				catch (FileNotFoundException e)
				{
					Log.error("[SettingsFile] File not found: " + fileName + ".ini: " + e.getMessage());
					subscriber.onError(e);
				}
				catch (UnsupportedEncodingException e)
				{
					Log.error("[SettingsFile] Bad encoding; please file a bug report: " + fileName + ".ini: " + e.getMessage());
					subscriber.onError(e);
				} finally
				{
					if (writer != null)
					{
						writer.close();
					}
				}

				subscriber.onNext(true);
				subscriber.onCompleted();
			}
		});
	}

	@NonNull
	private static File getSettingsFile(String fileName)
	{
		String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
		return new File(storagePath + "/dolphin-emu/Config/" + fileName + ".ini");
	}

	private static SettingSection sectionFromLine(String line)
	{
		String sectionName = line.substring(1, line.length() - 1);
		return new SettingSection(sectionName);
	}

	private static Setting settingFromLine(SettingSection current, String line)
	{
		String[] splitLine = line.split("=");

		String key = splitLine[0].trim();
		String value = splitLine[1].trim();

		try
		{
			int valueAsInt = Integer.valueOf(value);

			return new IntSetting(key, current, valueAsInt);
		}
		catch (NumberFormatException ex)
		{
		}

		try
		{
			float valueAsFloat = Float.valueOf(value);

			return new FloatSetting(key, current, valueAsFloat);
		}
		catch (NumberFormatException ex)
		{
		}

		switch (value)
		{
			case "True":
				return new BooleanSetting(key, current, true);
			case "False":
				return new BooleanSetting(key, current, false);
			default:
				return new StringSetting(key, current, value);
		}
	}

	private static void writeSection(PrintWriter writer, SettingSection section)
	{
		// Write the section header.
		String header = sectionAsString(section);
		writer.println(header);

		// Write this section's values.
		HashMap<String, Setting> settings = section.getSettings();
		Set<String> keySet = settings.keySet();

		for (String key : keySet)
		{
			Setting setting = settings.get(key);
			String settingString = settingAsString(setting);

			writer.println(settingString);
		}
	}

	private static String sectionAsString(SettingSection section)
	{
		return "[" + section.getName() + "]";
	}

	private static String settingAsString(Setting setting)
	{
		return setting.getKey() + " = " + setting.getValueAsString();
	}
}
