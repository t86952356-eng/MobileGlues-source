package com.fcl.plugin.mobileglues.settings

import com.google.gson.JsonParser
import org.junit.Assert.*
import org.junit.Test

class G99ProfileTest {
    @Test fun absentOrInvalidRequestIsOff() {
        for (value in listOf("{}", "{\"helioG99Test\":0}", "{\"helioG99Test\":2}", "{\"helioG99Test\":-1}", "{\"helioG99Test\":true}", "{\"helioG99Test\":\"bad\"}", "{\"helioG99Test\":\"1\"}", "{\"helioG99Test\":null}", "{\"helioG99Test\":[]}")) {
            assertFalse(MGConfigCodec.decode(JsonParser.parseString(value).asJsonObject).helioG99Test)
        }
    }
    @Test fun requestUsesNumericNativeWireFormatAndRoundTrips() {
        val config = MGConfig.Default.copy(helioG99Test = true)
        val encoded = MGConfigCodec.encode(config, null)
        assertEquals(1, encoded.get("helioG99Test").asInt)
        assertTrue(MGConfigCodec.decode(encoded).helioG99Test)
        assertEquals(config, MGConfigCodec.decode(encoded))
    }
    @Test fun disablingPreservesEveryManualSetting() {
        val original = MGConfig.Default.copy(angle = AngleConfig.ForceEnable,
            fsr1 = Fsr1Preset.Balanced, glslCache = GlslCacheSize.Limited(256))
        val toggled = original.copy(helioG99Test = true).copy(helioG99Test = false)
        assertEquals(original, toggled)
        assertEquals(original, MGConfigCodec.decode(MGConfigCodec.encode(toggled, null)))
    }
    @Test fun unknownSettingsSurviveAndProfileKeyIsOwned() {
        val raw = JsonParser.parseString("{\"helioG99Test\":1,\"futureKey\":42}").asJsonObject
        val foreign = MGConfigCodec.foreignKeysOf(raw)
        assertNull(foreign.get("helioG99Test"))
        val encoded = MGConfigCodec.encode(MGConfigCodec.decode(raw), foreign)
        assertEquals(42, encoded.get("futureKey").asInt)
        assertEquals(1, encoded.get("helioG99Test").asInt)
    }
}
